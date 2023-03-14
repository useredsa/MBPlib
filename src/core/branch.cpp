#include <iomanip>
#include <iostream>

#include "mbp/core/branch.hpp"

namespace mbp {

std::ostream& operator<<(std::ostream& os, Branch::OpCode opcode) {
  os << ((opcode & Branch::OpCode::IND) ? "IND " : "DIR ");
  os << ((opcode & Branch::OpCode::CND) ? "CND " : "UCD ");
  switch (opcode & Branch::OpCode::TYPE) {
    case Branch::JUMP:
      os << "JUMP";
      break;
    case Branch::CALL:
      os << "CALL";
      break;
    case Branch::RET:
      os << " RET";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Branch& b) {
  std::ios_base::fmtflags f(os.flags());
  os << std::hex;
  os << "0x" << std::setw(16) << std::setfill('0') << b.ip();
  os << " 0x" << std::setw(16) << std::setfill('0') << b.target();
  os << ' ' << b.opcode() << (b.isTaken() ? " TAKEN" : " NOT_TAKEN");
  os.flags(f);
  return os;
}

}  // namespace mbp
