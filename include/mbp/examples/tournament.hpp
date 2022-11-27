#ifndef MBP_EXAMPLES_TOURNAMENT_HPP_
#define MBP_EXAMPLES_TOURNAMENT_HPP_

#include <array>
#include <bitset>

#include "mbp/sim/predictor.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "mbp/utils/indexing.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

template <typename BP0, typename BP1, int T = 14>
struct BimodalTournament : Predictor {
  std::array<i2, (1 << T)> table;
  BP0 bp0;
  BP1 bp1;
  bool pred0, pred1, prediction, provider;

  uint64_t hash(uint64_t ip) const { return XorFold(ip, T); }

  bool predict(uint64_t ip) override {
    pred0 = bp0.predict(ip);
    pred1 = bp1.predict(ip);
    provider = table[hash(ip)] >= 0;
    prediction = provider ? pred1 : pred0;
    return prediction;
  }

  void train(const Branch& b) override {
    bp0.train(b);
    bp1.train(b);
    if (pred0 != pred1) {
      table[hash(b.ip())].sumOrSub(pred1 == b.isTaken());
    }
  }

  void track(const Branch& b) override {
    bp0.track(b);
    bp1.track(b);
  }

  json metadata_stats() const override {
    return {
        {"name", "MBPlib Bimodal Tournament"},
        {"log_table_size", T},
        {"predictor_0", bp0.metadata_stats()},
        {"predictor_1", bp1.metadata_stats()},
    };
  }
};

template <typename BP0, typename BP1, int H = 10, int T = 14>
struct GshareTournament : Predictor {
  static_assert(H + (T - (H % T)) < sizeof(unsigned long long) * 8);
  std::array<i3, (1 << T)> table;
  std::bitset<H> ghist;
  BP0 bp0;
  BP1 bp1;
  bool pred0, pred1, prediction, provider;

  uint64_t hash(uint64_t ip) const {
    return RoxFold(ip, T) ^ XorFold(ghist.to_ullong(), T);
  }

  bool predict(uint64_t ip) override {
    pred0 = bp0.predict(ip);
    pred1 = bp1.predict(ip);
    provider = table[hash(ip)] >= 0;
    prediction = provider ? pred1 : pred0;
    return prediction;
  }

  void train(const Branch& b) override {
    bp0.train(b);
    bp1.train(b);
    if (pred0 != pred1) {
      table[hash(b.ip())].sumOrSub(pred1 == b.isTaken());
    }
  }

  void track(const Branch& b) override {
    bp0.track(b);
    bp1.track(b);
    ghist <<= 1;
    ghist[0] = b.isTaken();
  }

  json metadata_stats() const override {
    return {
        {"name", "MBPlib Gshare Tournament"},
        {"history_length", H},
        {"log_table_size", T},
        {"predictor_0", bp0.metadata_stats()},
        {"predictor_1", bp1.metadata_stats()},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_TOURNAMENT_HPP_
