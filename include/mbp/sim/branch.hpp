#ifndef MBP_BRANCH_HPP_
#define MBP_BRANCH_HPP_

#include <cstdint>
#include <iosfwd>

namespace mbp {

/**
 * Branch representation (plus outcome) used by MBPlib.
 *
 * The branch type is determined by its opcode,
 * which is an abstract representation of possible branch instructions.
 * The opcode encodes the base type of the branch (JMP, RET or CALL)
 * as well as whether the branch is conditional and/or indirect.
 */
class Branch {
 public:
  /**
   * Enumeration type for the branch opcode.
   */
  enum OpCode {
    // Number of possible opcodes.
    NUMBER = 0b10000,
    // Bitmask corresponding to the base type.
    TYPE = 0b0011,
    // Bit for conditional branches.
    CND = 0b0100,
    // Bit for indirect branches.
    IND = 0b1000,
    // JMP base type.
    JMP = 0b0000,
    // RET (return from function) base type.
    RET = 0b0001,
    // CALL (function) base type.
    CALL = 0b0010,
  };

  Branch() = default;
  constexpr Branch(uint64_t ip, uint64_t target, OpCode opcode, uint8_t outcome)
      : ip_(ip), target_(target), opcode_(opcode), outcome_(outcome) {}

  // Returns the branch program address.
  constexpr uint64_t ip() const { return ip_; }
  // Returns the branch target program address.
  constexpr uint64_t target() const { return target_; }
  // Tells whether the branch is taken or not, according to the trace.
  constexpr bool isTaken() const { return outcome_; }
  // Returns the branch opcode.
  constexpr OpCode opcode() const { return opcode_; }
  // Tells whether the branch is conditional or not,
  // according to its opcode.
  constexpr bool isConditional() const { return (opcode_ & CND) != 0; }
  // Tells whether the branch is indirect or not,
  // according to its opcode.
  constexpr bool isIndirect() const { return (opcode_ & IND) != 0; }
  // Returns the base type of the branch opcode.
  constexpr OpCode type() const { return static_cast<OpCode>(opcode_ & TYPE); }

 private:
  uint64_t ip_;
  uint64_t target_;
  OpCode opcode_;
  uint8_t outcome_;

  friend std::ostream& operator<<(std::ostream& os, const Branch& b);
};

std::ostream& operator<<(std::ostream& os, Branch::OpCode opcode);
std::ostream& operator<<(std::ostream& os, const Branch& b);

}  // namespace mbp

#endif  // MBP_BRANCH_HPP_
