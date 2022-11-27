#ifndef MBP_EXAMPLES_BIMODAL_HPP_
#define MBP_EXAMPLES_BIMODAL_HPP_

#include <array>
#include <bitset>

#include "mbp/sim/predictor.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "mbp/utils/indexing.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

template <int T = 14>
struct Bimodal : Predictor {
  std::array<i2, (1 << T)> table;

  static constexpr uint64_t hash(uint64_t ip) { return ip & ((1ULL << T) - 1); }

  bool predict(uint64_t ip) override { return table[hash(ip)] >= 0; }

  void train(const Branch& b) override {
    table[hash(b.ip())].sumOrSub(b.isTaken());
  }

  void track(const Branch& b) override {}

  json metadata_stats() const override {
    return {
        {"name", "MBPlib Bimodal"},
        {"log_table_size", T},
    };
  }
};

template <int N, int T = 14>
struct Nmodal : Predictor {
  std::array<SatCtr<N>, (1 << T)> table;

  static constexpr uint64_t hash(uint64_t ip) { return ip & ((1ULL << T) - 1); }

  bool predict(uint64_t ip) override { return table[hash(ip)] >= 0; }

  void train(const Branch& b) override {
    table[hash(b.ip())].sumOrSub(b.isTaken());
  }

  void track(const Branch& b) override {}

  json metadata_stats() const override {
    return {
        {"name", "MBPlib Nmodal"},
        {"counter_bits", N},
        {"log_table_size", T},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_BIMODAL_HPP_
