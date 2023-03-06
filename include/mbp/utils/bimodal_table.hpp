#ifndef MBP_BIMODAL_TABLE_HPP_
#define MBP_BIMODAL_TABLE_HPP_

namespace mbp {

/**
 * An array of packed bimodal (signed 2-bit saturated) counters.
 *
 * Each counter uses exactly 2 bits.
 */
class BimodalArray {
 public:
  BimodalArray() : data_() {}

  BimodalArray(size_t n) : data_((n + 15) >> 4) {}

  BimodalArray(size_t n, int val) : data_() {
    assert(-2 <= val && val < 2);
    uint32_t broadcasted = ((val & 0b01) << 1) | ((val & 0b10) >> 1);
    for (int len = 2; len < 64; len <<= 1) {
      broadcasted |= broadcasted << len;
    }
    data_.assign((n + 1) >> 4, val);
  }

  BimodalArray(size_t n, i2 val) : BimodalArray(n, static_cast<int32_t>(val)){};

  BimodalArray(const BimodalArray&) = default;

  BimodalArray(BimodalArray&&) = default;

  ~BimodalArray() = default;

  inline bool prediction(size_t i) const {
    size_t idx = i >> 4;
    size_t off = (i & 0b1111) << 1;
    return (data_[idx] >> off) & uint32_t{1};
  }

  inline bool sign(size_t i) const { return prediction(i); }

  inline void update(size_t i, bool taken) {
    size_t idx = i >> 4;
    size_t off = (i & 0b1111) << 1;
    uint32_t dir = (data_[idx] >> off) & uint32_t{1};
    if (dir == taken) {
      data_[idx] |= size_t{1} << (off | 1);
    } else {
      uint32_t hys = (data_[idx] >> (off | 1)) & uint32_t{1};
      data_[idx] ^= uint32_t{1} << (off | hys);
    }
  }

 private:
  std::vector<uint32_t> data_;
};

}  // namespace mbp

#endif  // MBP_BIMODAL_TABLE_HPP_
