#include "mbp/sim/simulator.hpp"

#include <errno.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "nlohmann/json.hpp"

#include "mbp/sim/sbbt_reader.hpp"

namespace mbp {

const char* tracepath;
// Number of instructions that are not considered for metrics.
int64_t warmupInstrs;
// Number of instructions that are considered for metrics.
// Zero means that the whole trace must be considered.
int64_t simInstr;
// Instruction at which to stop reading the trace.
int64_t stopAtInstr;

void parse_args(int argc, char** argv) {
  if (argc != 2 && argc != 4) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " <trace> [<sim_instr> <warm_instr>]\n";
    exit(ERR_INPUT_DATA);
  }
  tracepath = argv[1];
  if (argc == 2) {
    warmupInstrs = 0;
    simInstr = 0;
  } else {
    char* endptr;
    if ((warmupInstrs = strtoll(argv[2], &endptr, 0)) < 0) {
      std::cerr << "<warm_instr> cannot be negative\n";
      exit(ERR_INPUT_DATA);
    }
    if (errno != 0 || endptr == argv[2]) {
      std::cerr << '\'' << argv[2] << '\'';
      std::cerr << " could not be parsed as integer for <warm_instr>\n";
      exit(ERR_INPUT_DATA);
    }
    if ((simInstr = strtoll(argv[3], &endptr, 0)) < 0) {
      std::cerr << "<sim_instr> cannot be negative\n";
      exit(ERR_INPUT_DATA);
    }
    if (errno != 0 || endptr == argv[3]) {
      std::cerr << '\'' << argv[3] << '\'';
      std::cerr << " could not be parsed as integer for <sim_instr>\n";
      exit(ERR_INPUT_DATA);
    }
  }
  if (simInstr != 0) {
    stopAtInstr = warmupInstrs + simInstr;
  } else {
    stopAtInstr = std::numeric_limits<int64_t>::max();
  }
}

int Simulation(int argc, char** argv) {
  int returnCode = 0;

  parse_args(argc, argv);

  SbbtReader trace{tracepath};
  struct BranchInfo {
    int64_t occurrences, misses;
  };
  std::unordered_map<uint64_t, BranchInfo> branchInfo;
  int64_t numBranches = 0;
  int64_t mispredictions = 0;

  auto startTime = std::chrono::high_resolution_clock::now();
  Branch b;
  int64_t instrNum = 0;
  while ((instrNum = trace.nextBranch(b)) < stopAtInstr) {
    bool predictedTaken = branchPredictor->predict(b.ip());
    if (b.isConditional()) {
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
  // The last branch instruction number should be close to
  // the number of simulation instructions, but not necesarilly equal,
  // because the simulation can end at a non-branch instruction.
  // If the trace must be exhausted then
  // the last instruction is the last branch cntained in the trace.
  // Otherwise take the number of instructions asked for simulation
  // as the denominator for metrics.
  // If you make all your experiments with the same simulation instructions,
  // this makes the mpki exactly proportional to the mispredictions.
  int64_t metricInstr =
      simInstr == 0 ? trace.lastInstrRead() - warmupInstrs : simInstr;
  if (simInstr != 0 && trace.eof()) {
    metricInstr = trace.lastInstrRead();
    std::cerr << "Exhausted the whole trace with the objective of\n";
    std::cerr << warmupInstrs << " warm up instructions,\n";
    std::cerr << simInstr << " simulation instructions.\n";
    std::cerr << "In the trace, only " << trace.lastInstrRead();
    std::cerr << " instructions were present\n";
    returnCode = ERR_EXHAUSTED_TRACE;
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
  for (int64_t keepsum = 0; keepidx < mostFailed.size(); ++keepidx) {
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
        {"accuracy",
         static_cast<double>(inf.occurrences - inf.misses) / inf.occurrences},
    };
    halfMispredictionsJson.emplace_back(std::move(j));
  }

  json output = {
      {"metadata",
       {
           {"simulator", "MBPlib std simulator"},
           {"simulator_version", "v0.5.0"},
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
           {"num_most_failed_branches", mostFailed.size()},
           {"simulation_time", simulationTime},
       }},
      {"predictor_statistics", branchPredictor->execution_stats()},
      {"most_failed", halfMispredictionsJson},
  };
  std::cout << std::setw(2) << output << std::endl;

  return returnCode;
}

}  // namespace mbp
