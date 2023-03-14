#include <iostream>
#include <mbp/examples/mbp_examples.hpp>
#include <mbp/sim/simulator.hpp>

#include "batage_specs.hpp"
#include "tage_specs.hpp"

auto branchPredictor0 = PREDICTOR0;
auto branchPredictor1 = PREDICTOR1;

int main(int argc, char** argv) {
  return mbp::CompareMain(argc, argv, {&branchPredictor0, &branchPredictor1});
}
