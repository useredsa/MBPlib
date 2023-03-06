#ifndef MBP_SATURATED_REG_HPP_
#define MBP_SATURATED_REG_HPP_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace mbp {

/**
 * Fixed width signed integer with saturated addition and substraction.
 */
template <int N>
class SatCtr {
 public:
  static constexpr int32_t kMinValue = -(1 << (N - 1));
  static constexpr int32_t kMaxValue = (1 << (N - 1)) - 1;

  SatCtr() = default;
  explicit constexpr SatCtr(int32_t x) : x_(x) {
    assert(kMinValue <= x_ && x_ <= kMaxValue);
  }

  explicit constexpr operator int32_t() const { return x_; }

  constexpr void sumOrSub(bool sum) {
    if (sum)
      ++(*this);
    else
      --(*this);
  }

  constexpr void operator++() { x_ = std::min(x_ + 1, kMaxValue); }
  constexpr void operator--() { x_ = std::max(x_ - 1, kMinValue); }
  constexpr void operator++(int) { ++(*this); }
  constexpr void operator--(int) { --(*this); }

  friend constexpr bool operator==(SatCtr l, SatCtr r) { return l.x_ == r.x_; }
  friend constexpr bool operator!=(SatCtr l, SatCtr r) { return l.x_ != r.x_; }
  friend constexpr bool operator<=(SatCtr l, SatCtr r) { return l.x_ <= r.x_; }
  friend constexpr bool operator>=(SatCtr l, SatCtr r) { return l.x_ >= r.x_; }
  friend constexpr bool operator<(SatCtr l, SatCtr r) { return l.x_ < r.x_; }
  friend constexpr bool operator>(SatCtr l, SatCtr r) { return l.x_ > r.x_; }
  friend constexpr bool operator==(SatCtr l, int32_t r) { return l.x_ == r; }
  friend constexpr bool operator!=(SatCtr l, int32_t r) { return l.x_ != r; }
  friend constexpr bool operator<=(SatCtr l, int32_t r) { return l.x_ <= r; }
  friend constexpr bool operator>=(SatCtr l, int32_t r) { return l.x_ >= r; }
  friend constexpr bool operator<(SatCtr l, int32_t r) { return l.x_ < r; }
  friend constexpr bool operator>(SatCtr l, int32_t r) { return l.x_ > r; }

 private:
  static_assert(N < 32, "Number of bits of SatCtr must be smaller than 32.");
  using T = typename std::conditional<
      (N <= 8), int8_t,
      typename std::conditional<(N <= 16), int16_t, int32_t>::type>::type;
  T x_;
};

/**
 * Fixed width unsigned integer with saturated addition and substraction.
 */
template <int N>
class USatCtr {
 public:
  static constexpr uint32_t kMaxValue = (1 << N) - 1;
  static constexpr uint32_t kMinValue = 0;

  USatCtr() = default;
  explicit constexpr USatCtr(uint32_t x) : x_(x) {
    assert(0 <= x_ && x_ <= kMaxValue);
  }

  constexpr explicit operator uint32_t() const {
    return static_cast<uint32_t>(x_);
  }

  constexpr void sumOrSub(bool sum) {
    if (sum)
      ++(*this);
    else
      --(*this);
  }

  constexpr void operator++() { x_ = std::min(x_ + 1U, kMaxValue); }
  constexpr void operator--() { x_ = std::min(x_ - 1U, kMinValue); }
  constexpr void operator++(int) { ++(*this); }
  constexpr void operator--(int) { --(*this); }

  friend constexpr bool operator==(USatCtr l, USatCtr r) {
    return l.x_ == r.x_;
  }
  friend constexpr bool operator!=(USatCtr l, USatCtr r) {
    return l.x_ != r.x_;
  }
  friend constexpr bool operator<=(USatCtr l, USatCtr r) {
    return l.x_ <= r.x_;
  }
  friend constexpr bool operator>=(USatCtr l, USatCtr r) {
    return l.x_ >= r.x_;
  }
  friend constexpr bool operator<(USatCtr l, USatCtr r) { return l.x_ < r.x_; }
  friend constexpr bool operator>(USatCtr l, USatCtr r) { return l.x_ > r.x_; }
  friend constexpr bool operator==(USatCtr l, uint32_t x) { return l.x_ == x; }
  friend constexpr bool operator!=(USatCtr l, uint32_t x) { return l.x_ != x; }
  friend constexpr bool operator<=(USatCtr l, uint32_t x) { return l.x_ <= x; }
  friend constexpr bool operator>=(USatCtr l, uint32_t x) { return l.x_ >= x; }
  friend constexpr bool operator<(USatCtr l, uint32_t x) { return l.x_ < x; }
  friend constexpr bool operator>(USatCtr l, uint32_t x) { return l.x_ > x; }

 private:
  static_assert(N < 32, "Number of bits of USatCtr must be smaller than 32.");
  using T = typename std::conditional<
      (N <= 8), uint8_t,
      typename std::conditional<(N <= 16), uint16_t, uint32_t>::type>::type;
  T x_;
};

/**
 * Batage-like Double Counter with Confidence Measure.
 */
template <int N>
class BatageCtr {
 private:
  using T = typename std::conditional<
      (N <= 8), uint8_t,
      typename std::conditional<(N <= 16), uint16_t, uint32_t>::type>::type;

 public:
  static constexpr uint32_t kMaxValue = (1 << N) - 1;

  BatageCtr() = default;
  explicit constexpr BatageCtr(T ctr0, T ctr1) : ctr_{ctr0, ctr1} {
    assert(0 <= ctr_[0] && ctr_[0] <= kMaxValue);
    assert(0 <= ctr_[1] && ctr_[1] <= kMaxValue);
  }

  /**
   * The prediction of the double counter,
   * which equals the difference between the conters.
   */
  constexpr int prediction() const { return ctr_[1] - ctr_[0]; }

  /**
   * The confidence on the prediction, belonging to {0, 1, 2}.
   *
   * A higher value means more confidence.
   *   2 = High Confidence,
   *   1 = Medium Confidence,
   *   0 = Low Confidence.
   */
  constexpr int confidenceLevel() const {
    int num = 1 + std::min(ctr_[0], ctr_[1]);
    int den = 2 + ctr_[0] + ctr_[1];
    int conf = den - 3 * num;
    if (conf > 0) return 2;
    return conf == 0;
  }

  /**
   * Returns whether the confidence in the prediction is very big.
   */
  constexpr bool isExcessivelyConfident() const {
    int num = 1 + std::min(ctr_[0], ctr_[1]);
    int den = 2 + ctr_[0] + ctr_[1];
    return 6 * num < den;
  }

  /**
   * Update the counter with a new outcome.
   */
  constexpr void update(int outcome) {
    if (ctr_[outcome] < kMaxValue) {
      ctr_[outcome] += 1;
    } else if (ctr_[1 - outcome] > 0) {
      ctr_[1 - outcome] -= 1;
    }
  }

  /**
   * Perform the decay operation,
   * reducing the confidence in the current prediction.
   */
  constexpr void decay() {
    if (ctr_[1] > ctr_[0]) ctr_[1]--;
    if (ctr_[0] > ctr_[1]) ctr_[0]--;
  }

 private:
  static_assert(N < 32, "Number of bits of BatageCtr must be smaller than 32.");
  std::array<T, 2> ctr_;
};

using i1 = SatCtr<1>;
using i2 = SatCtr<2>;
using i3 = SatCtr<3>;
using i4 = SatCtr<4>;
using i5 = SatCtr<5>;
using i6 = SatCtr<6>;
using i7 = SatCtr<7>;
using i8 = SatCtr<8>;

using u1 = USatCtr<1>;
using u2 = USatCtr<2>;
using u3 = USatCtr<3>;
using u4 = USatCtr<4>;
using u5 = USatCtr<5>;
using u6 = USatCtr<6>;
using u7 = USatCtr<7>;
using u8 = USatCtr<8>;

}  // namespace mbp

#endif  // MBP_SATURATED_REG_HPP_
