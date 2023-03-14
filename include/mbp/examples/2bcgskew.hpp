#ifndef MBP_EXAMPLES_2BCGSKEW_HPP_
#define MBP_EXAMPLES_2BCGSKEW_HPP_

#include <array>
#include <bitset>

#include "mbp/core/predictor.hpp"
#include "mbp/utils/indexing.hpp"
#include "mbp/utils/saturated_reg.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

/*
 * 2bc-gskew Predictor.
 *
 * A simplified version of the Alpha EV8's branch predictor.
 *
 * Reference: «Design Tradeoffs for the Alpha EV8 Conditional Branch Predictor»,
 * from André Seznec, Stephen Felix, Venkata Krishnan and Yiannakis Sazeides.
 */
template <int BT = 15, int G0T = 16, int G1T = 16, int MT = 15, int G0H = 17,
          int G1H = 27, int MH = 20>
struct Twobcgskew : Predictor {
  // Components
  std::array<i2, (1 << BT)> bim;
  std::array<i2, (1 << G0T)> g0;
  std::array<i2, (1 << G1T)> g1;
  std::array<i2, (1 << MT)> meta;
  std::bitset<std::max(std::max(G0H, G1H), MH)> ghist;
  // Saved Values
  uint64_t predictedIp, bimIdx, g0Idx, g1Idx, metaIdx;
  bool bimPred, g0Pred, g1Pred, metaPred;
  bool majorityVote, prediction, tracked = true;

  uint64_t bimHash(uint64_t ip) const { return XorFold(ip, BT); }

  uint64_t g0Hash(uint64_t ip) const {
    return XorFold(ip ^ (ghist.to_ullong() & ((1ULL << G0H) - 1)), G0T);
  }

  uint64_t g1Hash(uint64_t ip) const {
    return XorFold(ip ^ (ghist.to_ullong() & ((1ULL << G1H) - 1)), G1T);
  }

  uint64_t metaHash(uint64_t ip) const {
    return XorFold(ip ^ (ghist.to_ullong() & ((1ULL << MH) - 1)), MT);
  }

  bool predict(uint64_t ip) override {
    if (tracked == false && predictedIp == ip) return prediction;
    tracked = false;
    predictedIp = ip;

    bimIdx = bimHash(ip);
    g0Idx = g0Hash(ip);
    g1Idx = g1Hash(ip);
    metaIdx = metaHash(ip);
    bimPred = bim[bimIdx] >= 0;
    g0Pred = g0[g0Idx] >= 0;
    g1Pred = g1[g1Idx] >= 0;
    metaPred = meta[metaIdx] >= 0;
    majorityVote = g0Pred == g1Pred ? g0Pred : bimPred;
    prediction = metaPred ? majorityVote : bimPred;
    return prediction;
  }

  void train(const Branch& b) override {
    // To update prediction values if not done already.
    predict(b.ip());

    // Meta predictor is updated if both predictions are different.
    // It is strengthened if they are equal but the metaprediction was correct.
    if (majorityVote != bimPred) {
      meta[metaIdx].sumOrSub(majorityVote == b.isTaken());
    } else if (prediction == b.isTaken()) {
      meta[metaIdx].sumOrSub(metaPred);
    }

    // If the prediction was incorrect the update
    // can make the metaprediction change.
    // If the prediction is still incorrect, update everything.
    bool newMetapred = meta[metaIdx] >= 0;
    bool newPrediction = newMetapred ? majorityVote : bimPred;
    assert(b.isTaken() != prediction || newPrediction == prediction);
    if (newPrediction != b.isTaken()) {
      bim[bimIdx].sumOrSub(b.isTaken());
      g0[g0Idx].sumOrSub(b.isTaken());
      g1[g1Idx].sumOrSub(b.isTaken());
      return;
    }
    // If the prediction was correct
    // or it would have been correct with the new metaprediction,
    // strenghthen the provider components.

    // Do not update if all predictors agree.
    // Rationale: Limit number of strengthened counters
    // that make harder for other access to steal it.
    if (bimPred == g0Pred and bimPred == g1Pred) {
      // (We can only be here on a correct prediction.)
      return;
    }
    if (metaPred) {
      if (g0Pred == b.isTaken()) g0[g0Idx].sumOrSub(g0Pred);
      if (g1Pred == b.isTaken()) g1[g1Idx].sumOrSub(g1Pred);
      if (bimPred == b.isTaken()) bim[bimIdx].sumOrSub(bimPred);
    } else {
      bim[bimIdx].sumOrSub(bimPred);
    }
  }

  void track(const Branch& b) override {
    ghist <<= 1;
    ghist[0] = b.isTaken();
    tracked = true;
  }

  json metadata_stats() const override {
    return {
        {"name", "MBPlib 2bcgskew"},
        {"bimodal",
         {
             {"log_table_size", BT},
         }},
        {"gshare_0",
         {
             {"history_length", G0H},
             {"log_table_size", G0T},
         }},
        {"gshare_1",
         {
             {"history_length", G1H},
             {"log_table_size", G1T},
         }},
        {"metapredictor",
         {
             {"history_length", MH},
             {"log_table_size", MT},
         }},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_2BCGSKEW_HPP_
