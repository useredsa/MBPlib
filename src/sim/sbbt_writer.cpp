#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "mbp/sim/sbbt_writer.hpp"

namespace mbp {

static constexpr uint64_t OPCODE_MASK = 0x00F;
static constexpr uint64_t RESERVED_MASK = 0x7F0;
static constexpr uint64_t OUTCOME_MASK = 0x001;
static constexpr uint64_t MAX_IP_DIFF = 0xFFF;
static constexpr int IP_SHIFT = 12;
static constexpr int OUTCOME_SHIFT = 11;
static constexpr int OPCODE_SHIFT = 0;
static constexpr int IP_DIFF_SHIFT = 0;

struct SbbtBranch {
  uint64_t pkg0, pkg1;
};

static constexpr bool Is52BitsSignExtended(uint64_t ip) {
  constexpr uint64_t LAST_13_BITS = ((1ULL << 13) - 1) << 51;
  return (ip & LAST_13_BITS) == LAST_13_BITS || (ip & LAST_13_BITS) == 0;
}

SbbtWriter::SbbtWriter(const std::string& filename, uint64_t numInstructions,
                       uint64_t numBranches)
    : filename_(filename) {
  if (numInstructions == 0) {
    throw std::invalid_argument("SbbtWriter: numInstructions must be positive");
  }
  open();
  writeHeader(numInstructions, numBranches);
}

SbbtWriter::SbbtWriter(const std::string& filename) : filename_(filename) {
  open();
}

SbbtWriter::SbbtWriter(SbbtWriter&& o) {
  buffer_ = o.buffer_;
  pipe_ = o.pipe_;
  lastBranchInstrNum_ = o.lastBranchInstrNum_;
  numBranches_ = o.numBranches_;
  numInstructionsHeader_ = o.numInstructionsHeader_;
  numBranchesHeader_ = o.numBranchesHeader_;
  filename_ = o.filename_;
  bufferSize_ = o.bufferSize_;
  headerWritten_ = o.headerWritten_;

  o.pipe_ = nullptr;
  o.lastBranchInstrNum_ = 0;
  o.numBranches_ = 0;
  o.numInstructionsHeader_ = 0;
  o.numBranchesHeader_ = 0;
  o.filename_ = "";
  o.bufferSize_ = 0;
  o.headerWritten_ = false;
}

SbbtWriter::~SbbtWriter() {
  if (pipe_ == nullptr) return;
  try {
    close();
  } catch (std::exception const& e) {
    std::cerr << "SbbtWriter: exception catched in destructor. Erasing file.\n";
    std::cerr << e.what() << std::endl;
    remove(filename_.c_str());  // I don't care if this fails.
  }
}

void SbbtWriter::open() {
  if (filename_.size() < 9 ||
      filename_.substr(filename_.size() - 9) != ".sbbt.zst") {
    throw std::invalid_argument("SbbtWriter: file was '" + filename_ +
                                "' but extension has to be .sbbt.zst");
  }
  std::string cmd = "zstd --ultra -22 --force -o " + filename_;
  pipe_ = popen(cmd.c_str(), "w");
  if (pipe_ == nullptr) {
    throw std::runtime_error(std::strerror(errno));
  }
}

void SbbtWriter::writeHeader(uint64_t numInstructions, uint64_t numBranches) {
  assert(numInstructions != 0);
  assert(pipe_ != nullptr);
  SbbtHeader traceHeader;
  traceHeader.numInstructions = numInstructions;
  traceHeader.numBranches = numBranches;
  fwrite(&traceHeader, sizeof(traceHeader), 1, pipe_);
  if (std::ferror(pipe_)) {
    throw std::runtime_error(std::strerror(errno));
  }
  numInstructionsHeader_ = numInstructions;
  numBranchesHeader_ = numBranches;
  headerWritten_ = true;
}

void SbbtWriter::addBranch(uint64_t instrNum, uint64_t ip, uint64_t target,
                           bool outcome, uint8_t opcode) {
  if (numBranchesHeader_ != 0 && numBranches_ == numBranchesHeader_) {
    throw std::runtime_error(
        "SbbtWriter: You tried to add more branches "
        "than what you specified in the constructor");
  }
  if (!Is52BitsSignExtended(ip)) {
    throw std::invalid_argument(
        "SbbtWriter: ip must be a sign extension of a 52-bit ip");
  }
  if (!Is52BitsSignExtended(target)) {
    throw std::invalid_argument(
        "SbbtWriter: target must be a sign extension of a 52-bit ip");
  }
  uint64_t ipdiff = instrNum - lastBranchInstrNum_;
  if (ipdiff > MAX_IP_DIFF) {
    throw std::invalid_argument(
        "SbbtWriter: consecutive branches need to be separated by less than "
        "2^{12} instructions");
  }
  if ((opcode & TYPE) == 0b1100 || opcode & ~OPCODE_MASK) {
    throw std::invalid_argument("SbbtWriter: invalid opcode");
  }

  if (bufferSize_ == BUFFER_SIZE) writeBuffer();
  buffer_[bufferSize_++] = (ip << IP_SHIFT) |
                           (static_cast<uint64_t>(outcome) << OUTCOME_SHIFT) |
                           (static_cast<uint64_t>(opcode) << OPCODE_SHIFT);
  buffer_[bufferSize_++] = (target << IP_SHIFT) | (ipdiff << IP_DIFF_SHIFT);
  lastBranchInstrNum_ = instrNum;
  numBranches_ += 1;
}

void SbbtWriter::writeBuffer() {
  size_t written = fwrite(&buffer_, sizeof(uint64_t), bufferSize_, pipe_);
  for (size_t i = written; i < bufferSize_; ++i) {
    buffer_[i - written] = buffer_[i];
  }
  bufferSize_ -= written;
  if (std::ferror(pipe_)) {
    throw std::runtime_error(std::strerror(errno));
  }
}

void SbbtWriter::flush() {
  while (bufferSize_ > 0) writeBuffer();
}

void SbbtWriter::close() {
  close(headerWritten_ ? numInstructionsHeader_ : lastBranchInstrNum_);
}

void SbbtWriter::close(uint64_t numInstructions) {
  if (headerWritten_) {
    if (numInstructionsHeader_ != numInstructions) {
      throw std::runtime_error(
          "SbbtWriter: You specified a different number of instructions "
          "than what you specified in the constructor");
    }
    if (numBranchesHeader_ != numBranches_) {
      throw std::runtime_error(
          "SbbtWriter: You added less branches "
          "than what you specified in the constructor");
    }
  }
  flush();
  int zstdReturned = pclose(pipe_);
  pipe_ = nullptr;
  if (zstdReturned == -1) {
    throw std::runtime_error(std::strerror(errno));
  }
  if (zstdReturned != 0) {
    throw std::runtime_error("SbbtWriter: zstd returned " +
                             std::to_string(zstdReturned));
  }
  if (headerWritten_) return;

  std::string cmd = "zstd --decompress --stdout " + filename_;
  FILE* inputPipe = popen(cmd.c_str(), "r");
  if (inputPipe == nullptr) {
    throw std::runtime_error(strerror(errno));
  }
  std::string tmpfile = filename_ + ".tmp";
  cmd = "zstd --ultra -22 --force -o " + tmpfile;
  pipe_ = popen(cmd.c_str(), "w");
  if (pipe_ == nullptr) {
    throw std::runtime_error(strerror(errno));
  }

  writeHeader(numInstructions, numBranches_);
  static constexpr size_t READ_SIZE = 1 << 16;
  std::array<char, READ_SIZE> block;
  size_t read = 0;
  while ((read = fread(block.data(), 1, READ_SIZE, inputPipe)) != 0) {
    if (std::ferror(inputPipe)) {
      throw std::runtime_error(std::strerror(errno));
    }
    fwrite(block.data(), 1, read, pipe_);
    if (std::ferror(pipe_)) {
      throw std::runtime_error(std::strerror(errno));
    }
  }
  if (std::ferror(inputPipe)) {
    throw std::runtime_error(std::strerror(errno));
  }
  zstdReturned = pclose(inputPipe);
  if (zstdReturned == -1) {
    throw std::runtime_error(std::strerror(errno));
  }
  if (zstdReturned != 0) {
    throw std::runtime_error("SbbtWriter: zstd returned " +
                             std::to_string(zstdReturned));
  }
  zstdReturned = pclose(pipe_);
  pipe_ = nullptr;
  if (zstdReturned == -1) {
    throw std::runtime_error(std::strerror(errno));
  }
  if (zstdReturned != 0) {
    throw std::runtime_error("SbbtWriter: zstd returned " +
                             std::to_string(zstdReturned));
  }
  if (rename(tmpfile.c_str(), filename_.c_str())) {
    throw std::runtime_error(std::strerror(errno));
  }
}

}  // namespace mbp
