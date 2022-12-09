#ifndef MBP_EXAMPLES_GSHARE_HPP_
#define MBP_EXAMPLES_GSHARE_HPP_

#include <array>
#include <bitset>

#include "mbp/core/predictor.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "mbp/utils/indexing.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

/**
 * Gshare Predictor.
 *
 * @param H History length.
 * @param T Logarithm of table size.
 * @param IGNORE_UCD Whether to ignore unconditional branches for the history.
 */
template <int H = 15, int T = 14, bool IGNORE_UCD = false>
struct Gshare : Predictor {
  std::array<i2, (1 << T)> table;
  std::bitset<H> ghist;

  uint64_t hash(uint64_t ip) const {
    // Shift left the history to make the least significant address bits
    // be xored with the least amount of history bits.
    static_assert(H + (T - (H % T)) <= sizeof(unsigned long long) * 8);
    return XorFold(ip ^ (ghist.to_ullong() << (T - (H % T))), T);
  }

  bool predict(uint64_t ip) override { return table[hash(ip)] >= 0; }

  void train(const Branch& b) override {
    table[hash(b.ip())].sumOrSub(b.isTaken());
  }

  void track(const Branch& b) override {
    if (!b.isConditional() && IGNORE_UCD) return;
    ghist <<= 1;
    ghist[0] = b.isTaken();
  }

  json metadata_stats() const override {
    return {
        {"name", "MBPlib Gshare"},
        {"history_length", H},
        {"log_table_size", T},
        {"track_only_conditional", IGNORE_UCD},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_GSHARE_HPP_
