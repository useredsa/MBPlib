#ifndef MBP_INDEXING_HPP_
#define MBP_INDEXING_HPP_

#include <bitset>
#include <cassert>
#include <climits>
#include <cstdint>
#include <type_traits>

namespace mbp {

#ifdef __clang__

uint8_t bitreverse(uint8_t x) { return __builtin_bitreverse8(x); }
uint16_t bitreverse(uint16_t x) { return __builtin_bitreverse16(x); }
uint32_t bitreverse(uint32_t x) { return __builtin_bitreverse32(x); }
uint64_t bitreverse(uint64_t x) { return __builtin_bitreverse64(x); }

#else

template <typename T>
constexpr T bitreverse(T x) {
  T mask = ~T{0};
  for (int s = sizeof(T) * 4; s > 0; s >>= 1) {
    mask ^= mask << s;
    x = ((x >> s) & mask) | ((x << s) & ~mask);
  }
  return x;
}

#endif  // __clang__

constexpr uint64_t XorFold(uint64_t value, size_t width) {
  if (width == 0) return 0;
  uint64_t fold = 0;
  for (size_t i = 0; i < sizeof(uint64_t) * 8; i += width) {
    fold ^= value;
    value >>= width;
  }
  fold &= (1ULL << width) - 1;
  return fold;
}

constexpr uint64_t RoxFold(uint64_t value, size_t width) {
  if (width == 0) return 0;
  uint64_t fold = XorFold(value, 2 * width);
  fold ^= bitreverse(fold) >> (sizeof(uint64_t) * 8 - 2 * width);
  fold &= (1ULL << width) - 1;
  return fold;
}

/**
 * Maintains the XorFold of a a window of a stream of bits.
 *
 * The class uses the computed XorFold
 * as a base for computing the new XorFold
 * after new bits are added to the stream.
 * For that reason, this class is more efficient
 * than performing a XorFold each time.
 *
 * The most typical use case is
 * maintaining a XorFold of a window of the branch history
 * to use it to compute a hash.
 * For example, if winLen = 31 and foldLen = 7,
 * the xor fold associated to a branch history register h would be
 *   h[0..7] ^ h[7..14] ^ h[14..21] ^ h[21..28] ^ h[28..31].
 *
 * References: This folding technique was adapted from a predictor
 * from the first Championship Branch Prediction Workshop.
 * «A PPM-like, Tag-based Predictor, Pierre Michaud, IRISA/INRIA.»
 * https://jilp.org/cbp/Pierre.pdf
 * https://jilp.org/cbp/Agenda-and-Results.htm
 */
class BitStreamXorFold {
 public:
  /**
   * Default constructor.
   */
  BitStreamXorFold() = default;

  /**
   * Null initialized fold constructor.
   *
   * Creates a BitStreamXorFold of size foldLen for a window of size winLen
   * and sets the current fold to zero.
   */
  constexpr BitStreamXorFold(size_t winLen, size_t foldLen)
      : fold_(0),
        foldLen_(foldLen),
        bitOut_(foldLen != 0 ? winLen % foldLen : 0) {}

  /**
   * Value-based fold constructor.
   *
   * Creates a BitStreamXorFold of size foldLen for a window of size winLen
   * and sets the current fold to `foldVal`.
   */
  constexpr BitStreamXorFold(size_t winLen, size_t foldLen, uint64_t foldVal)
      : fold_(foldVal),
        foldLen_(foldLen),
        bitOut_(foldLen != 0 ? winLen % foldLen : 0) {}

  /**
   * Returns the current fold value.
   */
  constexpr uint64_t fold() const { return fold_; }

  /**
   * Updates the fold with the new bits of the bitstream.
   *
   * The fold is updated by considering that
   * new bits enter the window and some leave it.
   * The user must tell which were the bits leaving,
   * for which it is necessary to store
   * at least the window of bits of the bitstream.
   *
   * UB: Calling this function with more bits than the fold length
   * is not supported and should be treated as undefined behaviour.
   * UB: Calling this function with more bits 64 - foldLen
   * is also undefined behaviour.
   *
   * @param nbits    Number of bits both entering and leaving the window.
   * @param incoming Bits entering the window of the bitstream.
   *                 Lower bits correspond to newer bits.
   * @param outgoing Bits leaving the window of the bitstream.
   *                 Lower bits correspond to newer bits.
   */
  constexpr uint64_t shiftInAndOut(size_t nbits, uint32_t incoming,
                                   uint32_t outgoing) {
    // If the xor of the outgoing bits
    // is performed before completing the cyclic shift of the register
    // the outgoing bits that could be xored outside of the fold
    // will also end in their correct positions.
    fold_ <<= nbits;
    fold_ ^= incoming ^ (outgoing << bitOut_);
    fold_ ^= fold_ >> foldLen_;
    fold_ &= (uint64_t{1} << foldLen_) - 1;
    // Explanation (numbers represent instructions):
    // (1) requires foldLen_ + nbits < 64.
    // In order for (2) & (3) to give a correct result it is necessary that
    // (nbits - 1) + bitOut_ < 2*foldLen_,
    // which, along with the fact that bitOut_ is in the range [0, foldLen),
    // is guaranteed if
    // nbits < foldLen_ + 2.
    // The condition of the documentation is nbits < foldLen_ + 1.
    return fold_;
  }

 private:
  uint64_t fold_;
  uint32_t foldLen_;
  uint32_t bitOut_;
};

}  // namespace mbp

#endif  // MBP_INDEXING_HPP_
