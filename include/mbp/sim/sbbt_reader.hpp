#ifndef MBP_SBBT_READER_HPP_
#define MBP_SBBT_READER_HPP_

#include <array>
#include <string>

#include "mbp/sim/predictor.hpp"

namespace mbp {

/**
 * Trace reader for the SBBT format.
 */
class SbbtReader {
 public:
  SbbtReader(const SbbtReader& other) = delete;
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
   * Returns the instruction number of the last branch that was read.
   */
  constexpr int64_t lastInstrRead() const { return instrCtr_; }

 private:
  // Read size equals the Linux pipe buffer size, which is 4 pages.
  static constexpr size_t READ_SIZE = 1 << 16;
  static constexpr size_t SIZEOF_SBBT_BRANCH = 16;
  friend struct SbbtBranch;

  // The size of buffer_ is chosen so that
  // if we do not have enough bytes to return a branch to the user,
  // a reading of size READ_SIZE is always possible.
  std::array<char, READ_SIZE + SIZEOF_SBBT_BRANCH> buffer_;
  FILE* trace_ = nullptr;
  size_t bufferStart_ = 0;
  size_t bufferEnd_ = 0;
  int64_t instrCtr_ = 0;
};

}  // namespace mbp

#endif  // MBP_SBBT_READER_HPP_
