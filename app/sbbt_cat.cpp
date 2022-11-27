#include <iostream>
#include <limits>

#include "mbp/sim/sbbt_reader.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage " << argv[0] << " <trace_file>" << std::endl;
    return 1;
  }

  try {
    mbp::SbbtReader trace(argv[1]);
    mbp::Branch b{};
    while (trace.nextBranch(b) != std::numeric_limits<int64_t>::max()) {
      std::cout << trace.lastInstrRead() << ": " << b;
    }
  } catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
