#ifndef MBP_SIMULATOR_HPP_
#define MBP_SIMULATOR_HPP_

#include <string>
#include <vector>

#include "mbp/core/predictor.hpp"

namespace mbp {

// Exit code of the simulator when there is a problem with the input data.
constexpr int ERR_INPUT_DATA = 1;
// Exit code of the simulator when there was an error during simulation.
constexpr int ERR_SIMULATION_ERROR = 2;

struct SimArgs {
  std::string tracepath;
  int64_t warmupInstrs;
  int64_t simInstr;
  int64_t stopAtInstr;
};

SimArgs ParseCmdLineArgs(int argc, char** argv);

/**
 * Simulates a trace.
 */
json Simulate(Predictor* branchPredictor, const SimArgs& args);

/**
 * Simulates multiple predictors in parallel.
 */
json ParallelSim(const std::vector<Predictor*>& predictor, const SimArgs& args);

/**
 * Simulates a trace with two predictors and compares them.
 */
json Compare(std::array<Predictor*, 2> predictor, const SimArgs& args);

/**
 * Parses the command line arguments, calls mbp::Simulate and prints the output.
 */
int SimMain(int argc, char** argv, Predictor* branchPredictor);

/**
 * Parses the command line arguments, calls mbp::Compare and prints the output.
 */
int CompareMain(int argc, char** argv,
                std::array<Predictor*, 2> comparedPredictors);

}  // namespace mbp

#endif  // MBP_SIMULATOR_HPP_
