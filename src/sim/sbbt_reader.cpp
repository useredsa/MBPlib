#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include "mbp/sim/sbbt_reader.hpp"

namespace mbp {

struct SbbtBranch {
  unsigned opcode : 4;
  unsigned padding : 7;
  unsigned outcome : 1;
  unsigned long long ip : 52;
  unsigned ninstr : 12;
  unsigned long long target : 52;
};

static constexpr uint64_t sign_extend_ip(uint64_t ip) {
  constexpr uint64_t lastBit = uint64_t{1} << 51;
  // If (ip & lastBit) == 0, then ip is unchanged,
  // otherwise lastBit and the following bits get set to 1.
  return (ip ^ lastBit) - lastBit;
}

SbbtReader::SbbtReader(const std::string& trace)
    : buffer_{},
      trace_(nullptr),
      bufferStart_(0),
      bufferEnd_(0),
      instrCtr_(0),
      header_{} {
  // The trace can be either uncompressed...
  size_t last_dot = trace.find_last_of('.');
  // Note: we check that last_dot != 0 because we will write last_dot-1.
  if (last_dot == trace.npos || last_dot == 0) {
    throw std::invalid_argument("SbbtReader: file '" + trace +
                                "' does not have extension.");
  }
  std::string extension = trace.substr(last_dot);
  if (extension == ".sbbt") {
    trace_ = fopen(trace.c_str(), "r");
    if (trace_ == nullptr) {
      throw std::system_error(errno, std::generic_category(), "fopen failed");
    }
  } else {
    // Or compressed using a variety of tools: gzip, xz, zstd or lz4.
    last_dot = trace.find_last_of('.', last_dot - 1);
    if (last_dot == trace.npos) {
      throw std::invalid_argument(
          "SbbtReader: cannot recognize file type of '" + trace + "'.");
    }
    extension = trace.substr(last_dot);
    if (extension == ".sbbt.xz") {
      std::string cmd = "xz --decompress --keep --stdout " + trace;
      trace_ = popen(cmd.c_str(), "r");
      if (trace_ == nullptr) {
        throw std::system_error(errno, std::generic_category(),
                                "xz popen failed");
      }
    } else if (extension == ".sbbt.zst") {
      std::string cmd = "zstd --decompress --stdout --quiet " + trace;
      trace_ = popen(cmd.c_str(), "r");
      if (trace_ == nullptr) {
        throw std::system_error(errno, std::generic_category(),
                                "zstd popen failed");
      }
    } else if (extension == ".sbbt.lz4") {
      std::string cmd = "lz4 --decompress --stdout --quiet " + trace;
      trace_ = popen(cmd.c_str(), "r");
      if (trace_ == nullptr) {
        throw std::system_error(errno, std::generic_category(),
                                "lz4 popen failed");
      }
    } else if (extension == ".sbbt.gz") {
      std::string cmd = "gzip --decompress --stdout --keep " + trace;
      trace_ = popen(cmd.c_str(), "r");
      if (trace_ == nullptr) {
        throw std::system_error(errno, std::generic_category(),
                                "gzip popen failed");
      }
    } else {
      throw std::invalid_argument("SbbtReader: File type of '" + trace +
                                  "' not supported.");
    }
  }

  // Read header and check version number.
  while (bufferEnd_ - bufferStart_ < sizeof(SbbtHeader)) {
    if (std::feof(trace_)) {
      throw std::invalid_argument("SbbtReader: file '" + trace +
                                  "' is empty or too small.");
    }
    size_t len = std::fread(buffer_.data() + bufferEnd_, sizeof(char),
                            READ_SIZE, trace_);
    bufferEnd_ += len;
    if (std::ferror(trace_)) {
      throw std::runtime_error(std::strerror(errno));
    }
  }
  memcpy(&header_, buffer_.data(), sizeof(SbbtHeader));
  bufferStart_ += sizeof(SbbtHeader);
  uint64_t markWoVersion = header_.sbbtMark & SBBT_MARK_WO_VERSION_MASK;
  if (markWoVersion != SBBT_MARK_WO_VERSION) {
    std::stringstream stream;
    stream << "SbbtReader: Invalid header " << std::hex << markWoVersion
           << " (expected: " << SBBT_MARK_WO_VERSION << ").";
    throw std::invalid_argument(stream.str());
  }
  if (header_.sbbtMark != SBBT_MARK_WITH_VERSION) {
    std::stringstream stream;
    stream << "SbbtReader: Unsupported SBBT format version " << std::hex
           << (header_.sbbtMark & ~SBBT_MARK_WO_VERSION_MASK)
           << " (expected 0x000001).";
    throw std::invalid_argument(stream.str());
  }
}

SbbtReader::SbbtReader(SbbtReader&& other)
    : buffer_(other.buffer_),
      trace_(other.trace_),
      bufferStart_(other.bufferStart_),
      bufferEnd_(other.bufferEnd_),
      instrCtr_(other.instrCtr_),
      header_(other.header_) {
  other.trace_ = nullptr;
  other.bufferStart_ = 0;
  other.bufferEnd_ = 0;
  other.instrCtr_ = 0;
  other.header_ = {};
}

SbbtReader::~SbbtReader() {
  if (trace_ != nullptr) pclose(trace_);
}

bool SbbtReader::eof() const {
  return bufferEnd_ - bufferStart_ < sizeof(SbbtBranch) && std::feof(trace_);
}

int64_t SbbtReader::nextBranch(Branch& b) {
  // If the buffer does not contain a branch:
  // (1) move the partial branch bytes to the beginning of the buffer and
  // (2) perform a new read.
  while (bufferEnd_ - bufferStart_ < sizeof(SbbtBranch)) {
    if (std::feof(trace_)) {
      // The maximum value of int64_t must be returned
      // if there are not more branches.
      return std::numeric_limits<int64_t>::max();
    }
    for (size_t i = bufferStart_; i < bufferEnd_; ++i) {
      buffer_[i - bufferStart_] = buffer_[i];
    }
    bufferEnd_ -= bufferStart_;
    bufferStart_ = 0;
    size_t len = std::fread(buffer_.data() + bufferEnd_, sizeof(char),
                            READ_SIZE, trace_);
    bufferEnd_ += len;
    if (std::ferror(trace_)) {
      throw std::runtime_error(std::strerror(errno));
    }
  }
  static_assert(sizeof(SbbtBranch) == SbbtReader::SIZEOF_SBBT_BRANCH);
  SbbtBranch* srcBranch =
      reinterpret_cast<SbbtBranch*>(buffer_.data() + bufferStart_);
  bufferStart_ += sizeof(SbbtBranch);
  instrCtr_ += srcBranch->ninstr;
  b = Branch{sign_extend_ip(srcBranch->ip), sign_extend_ip(srcBranch->target),
             static_cast<Branch::OpCode>(srcBranch->opcode),
             static_cast<uint8_t>(srcBranch->outcome)};
  return instrCtr_;
}

}  // namespace mbp
