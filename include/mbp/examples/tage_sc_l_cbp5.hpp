#ifndef MBP_EXAMPLES_TAGE_SC_L_CBP5_HPP_
#define MBP_EXAMPLES_TAGE_SC_L_CBP5_HPP_

#include "mbp/sim/predictor.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

namespace tscl {

void ReInit();
bool GetPrediction(uint64_t);
void UpdatePredictor(uint64_t PC, const Branch& opType, bool resolveDir,
                     bool predDir, uint64_t branchTarget);
void TrackOtherInst(uint64_t PC, const Branch& opType, bool taken,
                    uint64_t branchTarget);

}  // namespace tscl

struct TageScL : Predictor {
  bool predicted;

  TageScL() { tscl::ReInit(); };

  bool predict(uint64_t ip) override {
    return predicted = tscl::GetPrediction(ip);
  }

  void train(const Branch& b) override {
    tscl::UpdatePredictor(b.ip(), b, b.isTaken(), predicted, b.target());
  }

  void track(const Branch& b) override {
    tscl::TrackOtherInst(b.ip(), b, b.isTaken(), b.target());
  }

  json metadata_stats() const override {
    return {
        {"name", "Tage-sc-l CBP5 ported for MBPlib"},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_TAGE_SC_L_CBP5_HPP_
