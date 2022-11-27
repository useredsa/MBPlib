#include "mbp/sim/branch.hpp"

#include <iostream>
#include <iomanip>

namespace mbp {

std::ostream& operator<<(std::ostream& os, Branch::OpCode opcode) {
  os << ((opcode & Branch::OpCode::IND) ? "IND-" : "DIR-");
  os << ((opcode & Branch::OpCode::CND) ? "CND-" : "UCD-");
  switch (opcode & Branch::OpCode::TYPE) {
    case Branch::JMP:
      os << "JMP";
      break;
    case Branch::CALL:
      os << "CALL";
      break;
    case Branch::RET:
      os << "RET";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Branch& b) {
  std::ios_base::fmtflags f(os.flags());
  os << std::hex;
  os << " 0x" << std::setw(8) << std::setfill('0') << b.ip();
  os << " 0x" << std::setw(8) << std::setfill('0') << b.target();
  os << ' ' << std::setw(4) << std::setfill(' ') << b.opcode();
  os << (b.isTaken() ? ": TAKEN\n" : ": NOT TAKEN\n");
  os.flags(f);
  return os;
}

}  // namespace mbp
