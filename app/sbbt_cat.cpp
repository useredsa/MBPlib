#include <iostream>
#include <iomanip>
#include <limits>

#include "mbp/sim/sbbt_reader.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::ordered_json;

int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) {
    std::cerr << "Usage " << argv[0]
              << " <trace_file> [(--no-header|--metadata)]" << std::endl;
    return 1;
  }
  bool printSbbtHeader = false;
  bool printDataHeader = true;
  if (argc == 3) {
    if (strcmp(argv[2], "--metadata") == 0) {
      printSbbtHeader = true;
    } else if (strcmp(argv[2], "--no-header") == 0) {
      printDataHeader = false;
    } else {
      std::cerr << "Unrecognized option: " << argv[2] << '\n';
      return 1;
    }
  }

  try {
    mbp::SbbtReader trace(argv[1]);
    mbp::Branch b{};

    if (printSbbtHeader) {
      json header = {
          {"num_instr", trace.numInstructions()},
          {"num_branches", trace.numBranches()},
      };
      std::cout << std::setw(2) << header << "\n";
      return 0;
    }

    if (printDataHeader) {
      std::cout << "┌─────────┬──────────┬──────────┬────────────┬─────────┐\n";
      std::cout << "│ InstNum │  Branch  │  Target  │   Opcode   │ Outcome │\n";
      std::cout << "└─────────┴──────────┴──────────┴────────────┴─────────┘\n";
    }
    while (trace.nextBranch(b) != std::numeric_limits<int64_t>::max()) {
      std::cout << std::setw(10) << std::setfill(' ') << trace.lastInstrRead()
                << ' ' << b << '\n';
    }
  } catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
    return 2;
  }
  return 0;
}
