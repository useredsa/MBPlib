#include <cstring>
#include <iomanip>
#include <iostream>

#include "mbp/sim/sbbt_reader.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::ordered_json;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage " << argv[0] << " <trace_file>" << std::endl;
    return 1;
  }

  try {
    mbp::SbbtReader trace(argv[1]);

    json header = {
        {"num_instr", trace.numInstructions()},
        {"num_branches", trace.numBranches()},
    };
    std::cout << std::setw(2) << header << "\n";
  } catch (std::exception const& e) {
    std::cerr << e.what() << std::endl;
    return 2;
  }
  return 0;
}
