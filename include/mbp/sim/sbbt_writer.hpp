#ifndef MBP_SBBT_WRITER_HPP_
#define MBP_SBBT_WRITER_HPP_

#include <array>
#include <cstdint>
#include <string>

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
 * Trace writer for the SBBT format.
 */
class SbbtWriter {
 public:
  static constexpr unsigned SBBT_VERSION_MAJOR = 1;
  static constexpr unsigned SBBT_VERSION_MINOR = 0;
  static constexpr unsigned SBBT_VERSION_PATCH = 0;
  static constexpr uint8_t CND = 0b0001U;
  static constexpr uint8_t IND = 0b0010U;
  static constexpr uint8_t TYPE = 0b1100U;
  static constexpr uint8_t JUMP = 0b0000U;
  static constexpr uint8_t CALL = 0b1000U;
  static constexpr uint8_t RET = 0b0100U;

  /**
   * Constructs an object SbbtWriter that will write to the file indicated.
   *
   * The number of branches should correspond with the number of times
   * that the method addBranch() will be called.
   * Otherwise, an exception will be thrown when calling addBranch() or close()
   * or, if close() is not called, the file will be erased by the destructor.
   * If you use this constructor and call close with the number of instructions,
   * the number should be the same.
   */
  SbbtWriter(const std::string& filename, uint64_t numInstructions,
             uint64_t numBranches);

  /**
   * Constructs an object SbbtWriter that will write to the file indicated.
   *
   * If you use this constructor,
   * you can call close(numInstructions)
   * after adding all the branches and before the object is destructed
   * to specify the corresponding number of instructions of the trace.
   * If you call close(),
   * the number of instructions will be assumed to be
   * the instruction number of the last branch.
   */
  SbbtWriter(const std::string& filename);

  // Deleted copy constructor.
  SbbtWriter(const SbbtWriter& other) = delete;

  /**
   * Move constructor.
   */
  SbbtWriter(SbbtWriter&& other);

  /**
   * Destructor.
   *
   * If the output file is still open, flushes the output and closes the file.
   *
   * If the total number of instructions was not given to the constructor,
   * it then needs to recompress the whole file to add the header,
   * assuming that the number of instructions
   * is the instruction number of the last branch.
   *
   * If an exception happens (for instance, the new file cannot be created),
   * then the constructor will TRY to erase the resulting file.
   * If you want to specify a different number of instructions for the header
   * or you want to handle exceptions yourself,
   * call close() before the object is destructed.
   */
  ~SbbtWriter();

  /**
   * Adds a branch to the trace.
   *
   * The branch is not directly written to the compression utility,
   * Branches are written in batches or when calling flush() or close().
   */
  void addBranch(uint64_t instrNum, uint64_t ip, uint64_t target, bool outcome,
                 uint8_t opcode);

  /**
   * Makes sure all the branches added are written to the compression utility.
   */
  void flush();

  /**
   * Completes the translation process.
   *
   * If the number of instructions was not specified in the constructor,
   * it will be assumed to be the instruction number of the last branch added.
   */
  void close();

  /**
   * Completes the translation process.
   *
   * If the total number of instructions was not given to the constructor,
   * the method needs to recompress the whole file to add the header.
   */
  void close(uint64_t numInstructions);

  /**
   * Number of branches added to the trace.
   */
  constexpr uint64_t numBranches() const { return numBranches_; }

 private:
  void open();
  void writeHeader(uint64_t numInstructions, uint64_t numBranches);
  void writeBuffer();

  struct SbbtHeader {
    // Reads as "SBBT\n" followed by 0x010000 and means SBBT v1.0.0.
    uint64_t sbbtMark = 0x0000010A54424253ULL;
    uint64_t numInstructions;
    uint64_t numBranches;
  };
  static_assert(sizeof(SbbtHeader) == 24);
  // Write size equals the Linux pipe buffer size, which is 4 pages.
  static constexpr int BUFFER_SIZE = 1 << 10;

  std::array<uint64_t, BUFFER_SIZE> buffer_;
  FILE* pipe_;
  uint64_t lastBranchInstrNum_ = 0;
  uint64_t numBranches_ = 0;
  uint64_t numInstructionsHeader_ = 0;
  uint64_t numBranchesHeader_ = 0;
  std::string filename_;
  size_t bufferSize_ = 0;
  bool headerWritten_ = false;
};

}  // namespace mbp

#endif  // MBP_SBBT_WRITER_HPP_
