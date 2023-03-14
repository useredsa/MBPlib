#ifndef MBP_SBBT_READER_HPP_
#define MBP_SBBT_READER_HPP_

#include <array>
#include <string>

#include "mbp/core/predictor.hpp"

#ifdef __GNUC__
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
              "System must be little endian\n");
#elif __clang__
static_assert(__LITTLE_ENDIAN__, "System must be little endian\n");
#else
#pragma message(                                 \
    "Cannot determine endianess of the system. " \
    "Check that the system is little endian.")
#endif

namespace mbp {

/**
 * Trace reader for the SBBT format.
 */
class SbbtReader {
 public:
  static constexpr unsigned SBBT_VERSION_MAJOR = 1;
  static constexpr unsigned SBBT_VERSION_MINOR = 0;
  static constexpr unsigned SBBT_VERSION_PATCH = 0;

  SbbtReader(const SbbtReader& other) = delete;
  SbbtReader(SbbtReader&& other);
  SbbtReader(const std::string& trace);
  ~SbbtReader();

  /**
   * Tells whether the end of file has been reached.
   */
  bool eof() const;

  /**
   * Reads the subsequent branch and returns its instruction number.
   *
   * @param b Variable to store the next branch contents.
   * @return the branch instruction number.
   * @return std::numeric_limits<int64_t>::max() if there are no more branches.
   */
  int64_t nextBranch(Branch& b);

  /**
   * Returns the number of instructions specified in the trace header.
   */
  constexpr uint64_t numInstructions() const { return header_.numInstructions; }

  /**
   * Returns the number of branches specified in the trace header.
   */
  constexpr uint64_t numBranches() const { return header_.numBranches; }

  /**
   * Returns the instruction number of the last branch that was read.
   */
  constexpr int64_t lastInstrRead() const { return instrCtr_; }

 private:
  struct SbbtHeader {
    uint64_t sbbtMark;
    uint64_t numInstructions;
    uint64_t numBranches;
  };
  static_assert(sizeof(SbbtHeader) == 24);
  static constexpr uint64_t SBBT_MARK_WO_VERSION_MASK = 0x000000FFFFFFFFFFULL;
  static constexpr uint64_t SBBT_MARK_WO_VERSION = 0x0000000A54424253ULL;
  static constexpr uint64_t SBBT_MARK_WITH_VERSION = 0x0000010A54424253ULL;
  // Read size equals the Linux pipe buffer size, which is 4 pages.
  static constexpr size_t READ_SIZE = 1 << 16;
  static constexpr size_t SIZEOF_SBBT_BRANCH = 16;

  // The size of buffer_ is chosen so that
  // if we do not have enough bytes to return a branch to the user,
  // a reading of size READ_SIZE is always possible.
  std::array<char, READ_SIZE + SIZEOF_SBBT_BRANCH> buffer_;
  FILE* trace_;
  size_t bufferStart_;
  size_t bufferEnd_;
  int64_t instrCtr_;
  SbbtHeader header_;
};

}  // namespace mbp

#endif  // MBP_SBBT_READER_HPP_
