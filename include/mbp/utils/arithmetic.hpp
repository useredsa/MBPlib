#ifndef MBP_ARITHMETIC_HPP_
#define MBP_ARITHMETIC_HPP_

#include <cassert>
#include <cstdint>

namespace mbp {

// TODO(useredsa): Make the functions templated.

constexpr uint32_t u32_sadd(uint32_t reg, uint32_t val, size_t width) {
  return std::min(reg + val, (uint32_t{1} << width) - 1);
}

constexpr uint32_t u32_ssub(uint32_t reg, uint32_t val) {
  return reg - std::min(reg, val);
}

constexpr uint32_t u32_supd(uint32_t reg, bool dir, size_t width) {
  return dir ? u32_sadd(reg, 1, width) : u32_ssub(reg, 1);
}

constexpr int32_t i32_sadd(int32_t reg, int32_t val, size_t width) {
  assert(val >= 0);
  return std::min(reg + val, (int32_t{1} << (width - 1)) - int32_t{1});
}

constexpr int32_t i32_ssub(int32_t reg, int32_t val, size_t width) {
  assert(val >= 0);
  return std::max(reg - val, -(int32_t{1} << (width - 1)));
}

constexpr int32_t i32_supd(int32_t reg, bool dir, size_t width) {
  if (dir) {
    return i32_sadd(reg, 1, width);
  } else {
    return i32_ssub(reg, 1, width);
  }
}

constexpr uint32_t u32_sll(uint32_t reg, uint32_t amm, size_t width) {
  reg <<= amm;
  reg &= (uint32_t{1} << width) - 1;
  return reg;
}

constexpr uint32_t u32_csll(uint32_t reg, uint32_t amm, size_t width) {
  reg <<= amm;
  reg ^= reg >> width;
  reg &= (uint32_t{1} << width) - 1;
  return reg;
}

constexpr int32_t u32_to_i32(uint32_t reg, size_t width) {
  int32_t zero = (width > 0 ? (int32_t{1} << (width - 1)) : 0);
  return static_cast<int32_t>(reg) - zero;
}

constexpr uint32_t i32_to_u32(int32_t reg, size_t width) {
  int32_t zero = (width > 0 ? (int32_t{1} << (width - 1)) : 0);
  return static_cast<uint32_t>(reg + zero);
}

}  // namespace mbp

#endif  // MBP_ARITHMETIC_HPP_
