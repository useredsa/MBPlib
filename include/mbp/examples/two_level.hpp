#ifndef MBP_EXAMPLES_TWO_LEVEL_HPP_
#define MBP_EXAMPLES_TWO_LEVEL_HPP_

#include <array>
#include <bitset>

#include "mbp/core/predictor.hpp"
#include "mbp/utils/indexing.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

/**
 * Two-Level Predictor.
 *
 * @param HLEN Number of bits of a branch history register (BHR).
 * @param HNUM Lognumber of BHRs.
 * @param HSET Lognumber of consecutive IPs that get assigned the same BHR.
 * @param PNUM Number of pattern history tables (PHT).
 * @param PSET Lognumber of consecutive IPs that get assigned the same PHT.
 *
 * This class can model all of the [GPS]A[gps] variants
 * by choosing appropiate values for the parameters.
 * G: HNUM = 0. HSET becomes irrelevant.
 * P: HNUM = n. HSET = 0.
 * S: HNUM = n. HSET = m.
 * g: PNUM = 0. PSET becomes irrelevant.
 * p: PNUM = x. PSET = 0.
 * s: PNUM = x. PSET = y.
 *
 * With regards to distributing the size dedicated to PHTs,
 * the best is to use a high number of history bits (HLEN)
 * and a small number of IP bits (PNUM) for indexing.
 * The optimal number of IPs per set (PSET)
 * seems to increase with the history length.
 * PSET = 3 => 8 IPs per block gave good results.
 * But this could be related to the fact
 * that the proportion of branch instructions is roughly 1/7.
 */
template <int HLEN, int HNUM, int HSET, int PNUM, int PSET>
struct TwoLevel : Predictor {
  std::array<std::bitset<HLEN>, (1 << HNUM)> bhr;
  std::array<i2, (1 << (PNUM + HLEN))> phr;

  uint64_t bhrHash(uint64_t ip) const {
    return (ip >> HSET) & ((1ULL << HNUM) - 1);
  }

  uint64_t phrHash(uint64_t ip) const {
    return (((ip >> PSET) & ((1ULL << PNUM) - 1)) << HLEN) |
           bhr[bhrHash(ip)].to_ullong();
  }

  bool predict(uint64_t ip) override { return phr[phrHash(ip)] >= 0; }

  void train(const Branch& b) override {
    phr[phrHash(b.ip())].sumOrSub(b.isTaken());
  }

  void track(const Branch& b) override {
    bhr[bhrHash(b.ip())] <<= 1;
    bhr[bhrHash(b.ip())][0] = b.isTaken();
  }

  json metadata_stats() const override {
    // clang-format off
    return {
        {"name", "MBPlib Two Level"},
        {"history_length", HLEN},
        {"log_num_bhr", HNUM},
        {"log_set_bhr", HSET},
        {"log_num_pht", PNUM},
        {"log_set_pht", PSET},
        {"log_table_size", PNUM + HLEN},
    };
    // clang-format on
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_TWO_LEVEL_HPP_
