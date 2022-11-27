#ifndef MBP_EXAMPLES_BATAGE_HPP_
#define MBP_EXAMPLES_BATAGE_HPP_

#include <array>
#include <vector>
#include <random>

#include "mbp/sim/predictor.hpp"
#include "mbp/utils/arithmetic.hpp"
#include "mbp/utils/indexing.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

struct Batage : Predictor {
  struct Entry {
    uint32_t tag;
    std::array<uint32_t, 2> ctr;
  };

  struct TableSpec {
    uint32_t ghistLen;
    uint32_t idxWidth;
    uint32_t tagWidth;
    uint32_t ctrWidth;
  };

  using Entries = std::vector<Entry*>;
  using Tags = std::vector<uint32_t>;

  constexpr int Prediction(Entry const* e) { return e->ctr[1] - e->ctr[0]; }

  constexpr int Confidence(Entry const* e) {
    int num = 1 + std::min(e->ctr[0], e->ctr[1]);
    int den = 2 + e->ctr[0] + e->ctr[1];
    int conf = den - 3 * num;
    if (conf > 0) return 2;
    return conf == 0;
  }

  constexpr bool ExcessiveHighConfidence(Entry const* e) {
    int num = 1 + std::min(e->ctr[0], e->ctr[1]);
    int den = 2 + e->ctr[0] + e->ctr[1];
    return 6 * num < den;
  }

  static constexpr void Update(Entry* e, int8_t outcome, size_t ctrWidth) {
    if (e->ctr[outcome] < (1U << (ctrWidth - 1)) - 1) {
      e->ctr[outcome] += 1;
    } else if (e->ctr[1 - outcome] > 0) {
      e->ctr[1 - outcome] -= 1;
    }
  }

  static constexpr void Decay(Entry* e) {
    if (e->ctr[1] > e->ctr[0]) e->ctr[1]--;
    if (e->ctr[0] > e->ctr[1]) e->ctr[0]--;
  }

  static constexpr int MAX_CAT = (1 << 15) << 4;
  // Minimum allocation probability is 1/MIN_AP.
  // Ideal is usually between 0.25 and 0.75.
  static constexpr int MIN_AP = 10;
  static constexpr int MAX_ALLOC_SKIP = 3;

  // Table Specification
  std::vector<uint32_t> ghistLen;
  std::vector<uint32_t> idxWidth;
  std::vector<uint32_t> tagWidth;
  std::vector<uint32_t> ctrWidth;
  // Random Number Generators
  std::mt19937 rng;
  std::uniform_int_distribution<std::mt19937::result_type> catRng;
  std::uniform_int_distribution<std::mt19937::result_type> decayRng;
  std::uniform_int_distribution<std::mt19937::result_type> skipRng;
  // Components
  std::vector<std::vector<Entry>> table;
  std::vector<BitStreamXorFold> idxFold;
  std::vector<BitStreamXorFold> tagFold;
  std::vector<char> ghist;
  size_t ghistIdx;
  int cat;
  // Saved Values
  Entries entry;
  Tags tag;
  uint64_t entriesIp;
  size_t prov;
  int confidence;
  bool prediction;
  bool updatedGhist;

  Batage(const std::vector<TableSpec>& specs, int seed)
      : ghistLen(),
        idxWidth(),
        tagWidth(),
        ctrWidth(),
        rng(seed),
        catRng(0, MIN_AP - 1),
        decayRng(0, 3),
        skipRng(1, MAX_ALLOC_SKIP),
        table(),
        idxFold(),
        tagFold(),
        ghist(),
        ghistIdx(0),
        cat(0),
        entry(specs.size()),
        tag(specs.size()),
        entriesIp(0),
        prov(0),
        confidence(0),
        prediction(false),
        updatedGhist(false) {
    unsigned maxGhistLen = 0;
    for (auto [hlen, iwidth, twidth, cwidth] : specs) {
      table.emplace_back(1 << iwidth);
      ghistLen.push_back(hlen);
      idxWidth.push_back(iwidth);
      tagWidth.push_back(twidth);
      ctrWidth.push_back(cwidth);
      idxFold.emplace_back(hlen, iwidth);
      tagFold.emplace_back(hlen, twidth);
      maxGhistLen = std::max(maxGhistLen, hlen);
    }
    ghist.resize(maxGhistLen + 1);
  }

  void computeEntriesAndTags(uint64_t ip) {
    for (size_t i = 0; i < table.size(); ++i) {
      uint64_t hash = XorFold(ip >> 2, idxWidth[i]) ^ idxFold[i].fold();
      entry[i] = &table[i][hash];
      unsigned shiftRight = std::max(tagWidth[i] - idxWidth[i], 0u);
      tag[i] = XorFold(ip << shiftRight, tagWidth[i]) ^ tagFold[i].fold();
    }
  }

  bool predict(uint64_t ip) {
    if (entriesIp == ip && updatedGhist == false) return prediction;
    entriesIp = ip;
    updatedGhist = false;

    computeEntriesAndTags(ip);
    prov = 0;
    prediction = Prediction(entry[0]) > 0;
    confidence = Confidence(entry[0]);
    for (size_t i = 1; i < entry.size(); ++i) {
      int pred = Prediction(entry[i]);
      int conf = Confidence(entry[i]);
      if (tag[i] == entry[i]->tag && conf >= confidence && pred != 0) {
        prov = i;
        prediction = pred > 0;
        confidence = conf;
      }
    }
    return prediction;
  }

  void train(const Branch& b) override {
    predict(b.ip());

    // Entries of bigger history length are updated.
    // Rationale: They were skipped because they were lower conf.
    size_t lastHit = prov;
    for (size_t i = prov + 1; i < entry.size(); ++i) {
      if (tag[i] == entry[i]->tag) {
        lastHit = i;
        Update(entry[i], b.isTaken(), ctrWidth[i]);
      }
    }
    // Alternate component is the last hitting component before provider.
    // (We are setting it to 0 when provider is also 0.)
    size_t alt = 0;
    for (size_t i = 0; i < prov; ++i) {
      if (entry[i]->tag == tag[i]) {
        alt = i;
      }
    }
    int altPred = Prediction(entry[alt]);
    int altCorrect = altPred != 0 && b.isTaken() == (altPred >= 0);
    int altConf = Confidence(entry[alt]);
    if (prov > 0 && confidence == 2 && altConf == 2 && altCorrect) {
      // If all of this happens at the same time
      // the provider entry is probably not so useful.
      Decay(entry[prov]);
    } else {
      // Otherwise update it normally.
      Update(entry[prov], b.isTaken(), ctrWidth[prov]);
    }
    // If the provider is not high confidence then update the alternate as well.
    if (prov > 0 && confidence != 2) {
      Update(entry[alt], b.isTaken(), ctrWidth[alt]);
    }

    // Allocate randomly on mispredictions.
    // The random probability is controlled by the variable cat as seen below.
    // The explanation is not trivial. You can check it on the original paper.
    if (prediction == b.isTaken()) return;
    int allocate = catRng(rng);
    if (allocate < cat * MIN_AP / (MAX_CAT + 1)) return;

    size_t skip = skipRng(rng);
    int mhc = 0;
    for (size_t i = lastHit + skip; i < entry.size(); ++i) {
      if (Confidence(entry[i]) == 2) {
        if (!ExcessiveHighConfidence(entry[i])) mhc += 1;
        if (decayRng(rng) == 0) {
          Decay(entry[i]);
        }
      } else {
        entry[i]->tag = tag[i];
        entry[i]->ctr[b.isTaken()] = 1;
        entry[i]->ctr[1 - b.isTaken()] = 0;

        cat += 3 - 4 * mhc;
        cat = std::max(0, std::min(cat, MAX_CAT));
        break;
      }
    }
  }

  void track(const Branch& b) override {
    ghistIdx = (ghistIdx == 0 ? ghist.size() : ghistIdx) - 1U;
    ghist[ghistIdx] = b.isTaken();
    for (size_t i = 0; i < table.size(); ++i) {
      unsigned outgoingBit = ghist[(ghistIdx + ghistLen[i]) % ghist.size()];
      idxFold[i].shiftInAndOut(1, b.isTaken(), outgoingBit);
      tagFold[i].shiftInAndOut(1, b.isTaken(), outgoingBit);
    }
    updatedGhist = true;
  }

  json metadata_stats() const override {
    std::vector<json> tableJson;
    for (size_t i = 0; i < table.size(); ++i) {
      // clang-format off
      tableJson.push_back(json{
        {"history_length", ghistLen[i]},
        {"log_size", idxWidth[i]},
        {"tag_width", tagWidth[i]},
        {"counter_width", ctrWidth[i]},
      });
      // clang-format on
    }
    return {
        {"name", "MBPlib Batage"},
        {"tables", tableJson},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_BATAGE_HPP_
