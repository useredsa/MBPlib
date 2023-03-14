#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "mbp/sim/sbbt_reader.hpp"

// clang-format off
static constexpr const char* HEADER =
  "┌──────────┬──────────────────┬──────────────────┬────────────┬─────────┐\n"
  "│ Inst Num │  Branch Address  │  Target Address  │   Opcode   │ Outcome │\n"
  "└──────────┴──────────────────┴──────────────────┴────────────┴─────────┘\n";
// clang-format on

int main(int argc, char** argv) {
  std::vector<std::string> traceFiles;
  bool printDataHeader = true;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--help") == 0) {
      std::cerr << "Usage: " << argv[0] << " [--no-header] <trace>..."
                << std::endl;
      return 1;
    } else if (strcmp(argv[i], "--no-header") == 0) {
      printDataHeader = false;
    } else {
      traceFiles.push_back(argv[i]);
    }
  }
  if (traceFiles.empty()) {
    std::cerr << "sbbt_cat: No trace given" << std::endl;
    return 1;
  }

  try {
    std::vector<mbp::SbbtReader> traces;
    for (const auto& file : traceFiles) {
      traces.emplace_back(file);
    }
    mbp::Branch b{};
    uint64_t instructionOffset = 0;
    if (printDataHeader) std::cout << HEADER;
    for (size_t i = 0; i < traces.size(); ++i) {
      while (traces[i].nextBranch(b) != std::numeric_limits<int64_t>::max()) {
        std::cout << std::setw(11) << std::setfill(' ')
                  << instructionOffset + traces[i].lastInstrRead() << ' ' << b
                  << '\n';
      }
      instructionOffset += traces[i].numInstructions();
    }
  } catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
    return 2;
  }
  return 0;
}
