#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "mbp/sim/sbbt_reader.hpp"
#include "mbp/sim/simulator.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

SimArgs ParseCmdLineArgs(int argc, char** argv) {
  SimArgs args;
  if (argc != 2 && argc != 4) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " <trace> [<warm_instr> <sim_instr>]\n";
    exit(ERR_INPUT_DATA);
  }
  args.tracepath = argv[1];
  if (argc == 2) {
    args.warmupInstrs = 0;
    args.simInstr = 0;
  } else {
    char* endptr;
    if ((args.warmupInstrs = strtoll(argv[2], &endptr, 0)) < 0) {
      std::cerr << "<warm_instr> cannot be negative\n";
      exit(ERR_INPUT_DATA);
    }
    if (errno != 0 || endptr == argv[2]) {
      std::cerr << '\'' << argv[2] << '\'';
      std::cerr << " could not be parsed as integer for <warm_instr>\n";
      exit(ERR_INPUT_DATA);
    }
    if ((args.simInstr = strtoll(argv[3], &endptr, 0)) < 0) {
      std::cerr << "<sim_instr> cannot be negative\n";
      exit(ERR_INPUT_DATA);
    }
    if (errno != 0 || endptr == argv[3]) {
      std::cerr << '\'' << argv[3] << '\'';
      std::cerr << " could not be parsed as integer for <sim_instr>\n";
      exit(ERR_INPUT_DATA);
    }
  }
  if (args.simInstr != 0) {
    args.stopAtInstr = args.warmupInstrs + args.simInstr;
  } else {
    args.stopAtInstr = std::numeric_limits<int64_t>::max();
  }
  return args;
}

/**
 * Note 0: On the denominator for the calculation of MPKI.
 *
 * The last branch instruction number should be close to
 * the number of simulation instructions, but not necesarilly equal,
 * because the simulation can end at a non-branch instruction.
 * If the trace must be exhausted then
 * the last instruction is the last branch contained in the trace.
 * Otherwise take the number of instructions asked for simulation
 * as the denominator for metrics.
 * If you make all your experiments with the same simulation instructions,
 * this makes the mpki exactly proportional to the mispredictions.
 */

constexpr size_t MAX_NUM_LISTED_BRANCHES = 20;

json Simulate(Predictor* branchPredictor, const SimArgs& args) {
  const auto& [tracepath, warmupInstrs, simInstr, stopAtInstr] = args;
  SbbtReader trace{tracepath};
  struct BranchInfo {
    int64_t occurrences, misses;
  };
  std::unordered_map<uint64_t, BranchInfo> branchInfo;
  int64_t numBranches = 0;
  int64_t mispredictions = 0;
  std::vector<std::string> errors;

  auto startTime = std::chrono::high_resolution_clock::now();
  Branch b;
  int64_t instrNum = 0;
  while ((instrNum = trace.nextBranch(b)) < stopAtInstr) {
    if (b.isConditional()) {
      bool predictedTaken = branchPredictor->predict(b.ip());
      branchPredictor->train(b);
      if (instrNum >= warmupInstrs) {
        numBranches += 1;
        branchInfo[b.ip()].occurrences += 1;
        bool mispredicted = predictedTaken != b.isTaken();
        mispredictions += mispredicted;
        branchInfo[b.ip()].misses += mispredicted;
      }
    }
    branchPredictor->track(b);
  }
  auto endTime = std::chrono::high_resolution_clock::now();
  double simulationTime =
      std::chrono::duration<double>(endTime - startTime).count();
  // See Note 0.
  int64_t metricInstr =
      simInstr == 0 ? trace.numInstructions() - warmupInstrs : simInstr;
  if (simInstr != 0 && trace.eof()) {
    std::string errMsg = "The trace did not contain " +
                         std::to_string(simInstr) + " instructions, only " +
                         std::to_string(trace.lastInstrRead());
    errors.emplace_back(errMsg);
  }

  // Most failed branches are defined as those
  // that together account for 1/2 of the mispredictions.
  std::vector<std::pair<uint64_t, BranchInfo>> mostFailed(branchInfo.begin(),
                                                          branchInfo.end());
  sort(mostFailed.begin(), mostFailed.end(),
       [](const auto& lhs, const auto& rhs) {
         return lhs.second.misses > rhs.second.misses;
       });
  size_t keepidx = 0;
  size_t lastidx = std::min(mostFailed.size(), MAX_NUM_LISTED_BRANCHES);
  for (int64_t keepsum = 0; keepidx < lastidx; ++keepidx) {
    if (2 * keepsum >= mispredictions) break;
    keepsum += mostFailed[keepidx].second.misses;
  }
  mostFailed.resize(keepidx);
  std::vector<json> halfMispredictionsJson;
  halfMispredictionsJson.reserve(keepidx);
  for (const auto& [ip, inf] : mostFailed) {
    json j{
        {"ip", ip},
        {"occurrences", inf.occurrences},
        {"mpki", 1000.0 * inf.misses / metricInstr},
    };
    halfMispredictionsJson.emplace_back(std::move(j));
  }

  json j = {
      {"metadata",
       {
           {"simulator", "MBPlib simulate"},
           {"simulator_version", "v0.6.0"},
           {"trace", tracepath},
           {"warmup_instr", warmupInstrs},
           {"simulation_instr", metricInstr},
           {"exhausted_trace", trace.eof()},
           {"num_conditonal_branches", numBranches},
           {"num_branch_instructions", branchInfo.size()},
           {"predictor", branchPredictor->metadata_stats()},
       }},
      {"metrics",
       {
           {"mpki", 1000.0 * mispredictions / metricInstr},
           {"mispredictions", mispredictions},
           {"accuracy",
            static_cast<double>(numBranches - mispredictions) / numBranches},
           {"simulation_time", simulationTime},
           {"num_most_failed_branches", mostFailed.size()},
       }},
      {"predictor_statistics", branchPredictor->execution_stats()},
      {"most_failed", halfMispredictionsJson},
      {"errors", errors},
  };
  return j;
}

json ParallelSim(const std::vector<Predictor*>& predictor,
                 const SimArgs& args) {
  const auto& [tracepath, warmupInstrs, simInstr, stopAtInstr] = args;
  SbbtReader trace{tracepath};
  int64_t numBranches = 0;
  std::vector<int64_t> mispredictions(predictor.size());
  std::vector<std::string> errors;

  auto startTime = std::chrono::high_resolution_clock::now();
  Branch b;
  int64_t instrNum = 0;
  while ((instrNum = trace.nextBranch(b)) < stopAtInstr) {
    if (b.isConditional()) {
      if (instrNum >= warmupInstrs) {
        numBranches += 1;
      }
      for (size_t i = 0; i < predictor.size(); ++i) {
        bool predictedTaken = predictor[i]->predict(b.ip());
        predictor[i]->train(b);
        predictor[i]->track(b);
        if (instrNum >= warmupInstrs) {
          mispredictions[i] += predictedTaken != b.isTaken();
        }
      }
    } else {
      for (size_t i = 0; i < predictor.size(); ++i) {
        predictor[i]->track(b);
      }
    }
  }
  auto endTime = std::chrono::high_resolution_clock::now();
  double simulationTime =
      std::chrono::duration<double>(endTime - startTime).count();
  // See Note 0.
  int64_t metricInstr =
      simInstr == 0 ? trace.numInstructions() - warmupInstrs : simInstr;
  if (simInstr != 0 && trace.eof()) {
    std::string errMsg = "The trace did not contain " +
                         std::to_string(simInstr) + " instructions, only " +
                         std::to_string(trace.lastInstrRead());
    errors.emplace_back(errMsg);
  }

  std::vector<json> results;
  results.reserve(predictor.size());
  for (size_t i = 0; i < predictor.size(); ++i) {
    json j = {
        {"predictor", predictor[i]->metadata_stats()},
        {"metrics",
         {
             {"mpki", 1000.0 * mispredictions[i] / metricInstr},
             {"mispredictions", mispredictions[i]},
             {"accuracy", static_cast<double>(numBranches - mispredictions[i]) /
                              numBranches},
         }},
        {"predictor_statistics", predictor[i]->execution_stats()},
    };
    results.emplace_back(std::move(j));
  }
  json j = {
      {"metadata",
       {
           {"simulator", "MBPlib parallel simulate"},
           {"simulator_version", "v0.1.0"},
           {"trace", tracepath},
           {"warmup_instr", warmupInstrs},
           {"simulation_instr", metricInstr},
           {"exhausted_trace", trace.eof()},
           {"num_conditonal_branches", numBranches},
       }},
      {"simulation_time", simulationTime},
      {"results", results},
      {"errors", errors},
  };
  return j;
}

json Compare(std::array<Predictor*, 2> predictor, const SimArgs& args) {
  const auto& [tracepath, warmupInstrs, simInstr, stopAtInstr] = args;
  SbbtReader trace{tracepath};
  using BranchInfo = std::array<int64_t, 4>;
  std::unordered_map<uint64_t, BranchInfo> branchInfo;
  std::vector<std::string> errors;

  auto startTime = std::chrono::high_resolution_clock::now();
  Branch b;
  int64_t instrNum = 0;
  while ((instrNum = trace.nextBranch(b)) < stopAtInstr) {
    if (b.isConditional()) {
      std::array<bool, 2> pred = {
          predictor[0]->predict(b.ip()),
          predictor[1]->predict(b.ip()),
      };
      predictor[0]->train(b);
      predictor[1]->train(b);
      if (instrNum >= warmupInstrs) {
        std::array<int, 2> wasMisp = {pred[0] != b.isTaken(),
                                      pred[1] != b.isTaken()};
        int index = (wasMisp[1] << 1) | wasMisp[0];
        branchInfo[b.ip()][index] += 1;
      }
    }
    predictor[0]->track(b);
    predictor[1]->track(b);
  }
  auto endTime = std::chrono::high_resolution_clock::now();
  double simulationTime =
      std::chrono::duration<double>(endTime - startTime).count();
  // See Note 0.
  int64_t metricInstr =
      simInstr == 0 ? trace.numInstructions() - warmupInstrs : simInstr;
  if (simInstr != 0 && trace.eof()) {
    std::string errMsg = "The trace did not contain " +
                         std::to_string(simInstr) + " instructions, only " +
                         std::to_string(trace.lastInstrRead());
    errors.emplace_back(errMsg);
  }

  std::vector<std::pair<uint64_t, BranchInfo>> simInfo(branchInfo.begin(),
                                                       branchInfo.end());
  int64_t numBranches = 0;
  std::array<int64_t, 2> mispredictions = {0, 0};
  int64_t mispredictionsDiff = 0;
  for (const auto& [ip, info] : simInfo) {
    numBranches += info[0b00] + info[0b01] + info[0b10] + info[0b11];
    mispredictions[0] += info[0b01] + info[0b11];
    mispredictions[1] += info[0b10] + info[0b11];
    mispredictionsDiff += std::abs(info[0b10] - info[0b01]);
  }
  // Most failed branches are defined as those
  // that together account for 1/2 of the mpki difference.
  std::vector<json> mostFailedJson;
  sort(simInfo.begin(), simInfo.end(), [](const auto& lhs, const auto& rhs) {
    return std::abs(lhs.second[0b10] - lhs.second[0b01]) >
           std::abs(rhs.second[0b10] - rhs.second[0b01]);
  });
  size_t keepidx = 0;
  size_t lastidx = std::min(simInfo.size(), MAX_NUM_LISTED_BRANCHES);
  for (int64_t keepsum = 0; keepidx < lastidx; ++keepidx) {
    if (2 * keepsum >= mispredictionsDiff) break;
    const auto& info = simInfo[keepidx].second;
    std::array<double, 2> mpki = {
        1000.0 * (info[0b01] + info[0b11]) / metricInstr,
        1000.0 * (info[0b10] + info[0b11]) / metricInstr,
    };
    int64_t mispDiff = std::abs(info[0b10] - info[0b01]);
    keepsum += mispDiff;
    json j{
        {"ip", simInfo[keepidx].first},
        {"occurrences", info[0b00] + info[0b01] + info[0b10] + info[0b11]},
        {"mpki", mpki},
        {"mpki_difference", 1000.0 * mispDiff / metricInstr},
    };
    mostFailedJson.emplace_back(std::move(j));
  }
  simInfo.resize(keepidx);

  json j = {
      {"metadata",
       {
           {"simulator", "MBPlib compare"},
           {"simulator_version", "v0.1.0"},
           {"trace", tracepath},
           {"warmup_instr", warmupInstrs},
           {"simulation_instr", metricInstr},
           {"exhausted_trace", trace.eof()},
           {"num_conditonal_branches", numBranches},
           {"num_branch_instructions", branchInfo.size()},
           {"predictors",
            {
                predictor[0]->metadata_stats(),
                predictor[1]->metadata_stats(),
            }},
       }},
      {"metrics",
       {
           {"mpki",
            {
                1000.0 * mispredictions[0] / metricInstr,
                1000.0 * mispredictions[1] / metricInstr,
            }},
           {"mpki_difference", 1000.0 * mispredictionsDiff / metricInstr},
           {"mispredictions", mispredictions},
           {"simulation_time", simulationTime},
           {"num_most_failed_branches", simInfo.size()},
       }},
      {"predictor_statistics",
       {
           predictor[0]->execution_stats(),
           predictor[1]->execution_stats(),
       }},
      {"most_failed", mostFailedJson},
      {"errors", errors},
  };
  return j;
}

int SimMain(int argc, char** argv, Predictor* branchPredictor) {
  json output = Simulate(branchPredictor, ParseCmdLineArgs(argc, argv));
  std::cout << std::setw(2) << output << std::endl;
  return output["errors"].empty() ? 0 : ERR_SIMULATION_ERROR;
}

int CompareMain(int argc, char** argv,
                std::array<Predictor*, 2> comparedPredictors) {
  auto args = ParseCmdLineArgs(argc, argv);
  mbp::json output = mbp::Compare(comparedPredictors, args);
  std::cout << std::setw(2) << output << std::endl;
  return output["errors"].empty() ? 0 : ERR_SIMULATION_ERROR;
}
}  // namespace mbp
