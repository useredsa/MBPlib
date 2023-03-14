#include <mbp/examples/mbp_examples.hpp>
#include <mbp/sim/simulator.hpp>

#include "batage_specs.hpp"
#include "tage_specs.hpp"

static auto branchPredictor = PREDICTOR;

int main(int argc, char** argv) {
  return mbp::SimMain(argc, argv, &branchPredictor);
}
