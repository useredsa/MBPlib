#include "mbp/sim/sbbt_reader.hpp"

#include <errno.h>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>

namespace mbp {

struct SbbtBranch {
  unsigned ninstr : 12;
  unsigned long long ip : 52;
  unsigned char padding : 7;
  unsigned type : 2;
  unsigned conditional : 1;
  unsigned direct : 1;
  unsigned outcome : 1;
  unsigned long long target : 52;
};

SbbtReader::SbbtReader(const std::string& trace) {
  // The trace can be either uncompressed...
  size_t last_dot = trace.find_last_of('.');
  // Note: we check that last_dot != 0 because we will write last_dot-1.
  if (last_dot == trace.npos || last_dot == 0) {
    throw std::invalid_argument("Cannot recognize trace format (no entension)");
  }
  std::string extension = trace.substr(last_dot);
  if (extension == ".sbbt") {
    trace_ = fopen(trace.c_str(), "r");
    if (trace_ == nullptr) {
      throw std::system_error(errno, std::generic_category(),
                              "trace fopen failed");
    }
    return;
  }
  // Or compressed using a variety of tools: gzip, xz, zstd or lz4.
  last_dot = trace.find_last_of('.', last_dot - 1);
  if (last_dot == trace.npos) {
    throw std::invalid_argument("Cannot recognize trace format (not sbbt)");
  }
  extension = trace.substr(last_dot);
  if (extension == ".sbbt.xz") {
    std::string cmd = "xz --decompress --keep --stdout " + trace;
    trace_ = popen(cmd.c_str(), "r");
    if (trace_ == nullptr) {
      throw std::system_error(errno, std::generic_category(),
                              "xz popen failed");
    }
    return;
  }
  if (extension == ".sbbt.zst") {
    std::string cmd = "zstd --decompress --stdout --quiet " + trace;
    trace_ = popen(cmd.c_str(), "r");
    if (trace_ == nullptr) {
      throw std::system_error(errno, std::generic_category(),
                              "zstd popen failed");
    }
    return;
  }
  if (extension == ".sbbt.lz4") {
    std::string cmd = "lz4 --decompress --stdout --quiet " + trace;
    trace_ = popen(cmd.c_str(), "r");
    if (trace_ == nullptr) {
      throw std::system_error(errno, std::generic_category(),
                              "lz4 popen failed");
    }
    return;
  }
  if (extension == ".sbbt.gz") {
    std::string cmd = "gzip --decompress --stdout --keep " + trace;
    trace_ = popen(cmd.c_str(), "r");
    if (trace_ == nullptr) {
      throw std::system_error(errno, std::generic_category(),
                              "gzip popen failed");
    }
    return;
  }
  throw std::invalid_argument("Cannot recognize trace format");
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
  // TODO(useredsa) optimize trace layout to make a simple copy
  Branch::OpCode opcode = static_cast<Branch::OpCode>(srcBranch->type);
  if (srcBranch->conditional)
    opcode = static_cast<Branch::OpCode>(opcode | Branch::CND);
  if (!srcBranch->direct)
    opcode = static_cast<Branch::OpCode>(opcode | Branch::IND);
  b = Branch(srcBranch->ip, srcBranch->target, opcode,
             static_cast<uint8_t>(srcBranch->outcome));
  return instrCtr_;
}

}  // namespace mbp
