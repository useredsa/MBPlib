#ifndef MBP_SIMULATOR_HPP_
#define MBP_SIMULATOR_HPP_

#include "mbp/sim/predictor.hpp"

namespace mbp {

// Exit code of the simulator when there is a problem with the input data.
constexpr int ERR_INPUT_DATA = 1;
// Exit code of the simulator when
// the trace does not contain the number of instructions specified by the user.
constexpr int ERR_EXHAUSTED_TRACE = 2;

/**
 * Predictor class evaluated by the simulator.
 *
 * This variable must be initalized by the user
 * or they will get a linker error.
 */
extern Predictor* const branchPredictor;

/**
 * Main program for the simulator.
 *
 * It should be called after mbp::branchPredictor has been initialized.
 */
int Simulation(int argc, char** argv);

}  // namespace mbp

#endif  // MBP_SIMULATOR_HPP_
