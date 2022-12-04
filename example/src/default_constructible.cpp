#include <mbp/examples/mbp_examples.hpp>
#include <mbp/sim/simulator.hpp>

BP_CLASS branchPredictor{};

int main(int argc, char** argv) {
  return mbp::SimMain(argc, argv, &branchPredictor);
}
