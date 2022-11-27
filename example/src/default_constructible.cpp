#include "mbp/examples/mbp_examples.hpp"
#include "mbp/sim/simulator.hpp"

BP_CLASS branchPredictorImpl{};

mbp::Predictor* const mbp::branchPredictor = &branchPredictorImpl;

int main(int argc, char** argv) { return mbp::Simulation(argc, argv); }
