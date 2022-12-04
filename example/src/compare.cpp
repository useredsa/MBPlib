#include <iostream>
#include <mbp/examples/mbp_examples.hpp>
#include <mbp/sim/simulator.hpp>

BP_CLASS0 branchPredictor0{};
BP_CLASS1 branchPredictor1{};

int main(int argc, char** argv) {
  return mbp::CompareMain(argc, argv, {&branchPredictor0, &branchPredictor1});
}
