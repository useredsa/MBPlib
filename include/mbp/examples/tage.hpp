#ifndef MBP_EXAMPLES_TAGE_HPP_
#define MBP_EXAMPLES_TAGE_HPP_

#include <vector>

#include "mbp/core/predictor.hpp"
#include "mbp/utils/arithmetic.hpp"
#include "mbp/utils/indexing.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

struct Tage : Predictor {
  struct Entry {
    uint32_t tag;
    int32_t ctr;
    uint32_t utl;
  };

  struct TableSpec {
    unsigned ghistLen;
    unsigned idxWidth;
    unsigned tagWidth;
    unsigned ctrWidth;
    unsigned utlWidth;
  };

  static constexpr int LOG_META_SIZE = 8;
  using Entries = std::vector<Entry*>;
  using Tags = std::vector<uint32_t>;

  // Table Specification
  std::vector<uint32_t> ghistLen;
  std::vector<uint32_t> idxWidth;
  std::vector<uint32_t> tagWidth;
  std::vector<uint32_t> ctrWidth;
  std::vector<uint32_t> utlWidth;
  // Components
  std::vector<std::vector<Entry>> table;
  std::vector<BitStreamXorFold> idxFold;
  std::vector<BitStreamXorFold> tagFold;
  std::vector<char> ghist;
  size_t ghistIdx;
  std::array<i5, (1 << LOG_META_SIZE)> meta;
  // Saved Values
  Entries entry;
  Tags tag;
  int m0, m1, prov;
  bool predM0, predM1, m0IsWeak, m1IsWeak, prediction;
  uint64_t entriesIp;
  bool updatedGhist;

  Tage(const std::vector<TableSpec>& specs)
      : ghistLen(),
        idxWidth(),
        tagWidth(),
        ctrWidth(),
        utlWidth(),
        table(),
        idxFold(),
        tagFold(),
        ghist(),
        ghistIdx(0),
        entry(specs.size()),
        tag(specs.size()),
        entriesIp(0),
        updatedGhist(true) {
    unsigned maxGhistLen = 0;
    for (auto [hlen, iwidth, twidth, cwidth, uwidth] : specs) {
      table.emplace_back(1 << iwidth);
      ghistLen.push_back(hlen);
      idxWidth.push_back(iwidth);
      tagWidth.push_back(twidth);
      ctrWidth.push_back(cwidth);
      utlWidth.push_back(uwidth);
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

  uint64_t metahash() const {
    return XorFold(m0, LOG_META_SIZE - 1) << 1 | m1IsWeak;
  }

  bool predict(uint64_t ip) override {
    if (entriesIp == ip && updatedGhist == false) return prediction;
    entriesIp = ip;
    updatedGhist = false;

    computeEntriesAndTags(ip);
    // Compute furthest and second furthest matching components
    m0 = entry.size() - 1;
    while (m0 > 0 && entry[m0]->tag != tag[m0]) --m0;
    predM0 = entry[m0]->ctr >= 0;
    m0IsWeak = entry[m0]->ctr == -1 || entry[m0]->ctr == 0;
    m1 = m0 - 1;
    while (m1 > 0 && entry[m1]->tag != tag[m1]) --m1;
    predM1 = m0 > 0 ? entry[m1]->ctr >= 0 : 0;
    m1IsWeak = m0 > 0 && (entry[m1]->ctr == -1 || entry[m1]->ctr == 0);
    // Choose between both
    prov = m0 > 0 && m0IsWeak && meta[metahash()] >= 0 ? m1 : m0;
    return prediction = (entry[prov]->ctr >= 0);
  }

  void train(const Branch& b) override {
    predict(b.ip());

    entry[m0]->ctr = i32_supd(entry[m0]->ctr, b.isTaken(), ctrWidth[m0]);
    entry[m0]->utl =
        u32_supd(entry[m0]->utl, prediction == b.isTaken(), utlWidth[m0]);
    bool m0remainsWeak = entry[m0]->ctr == -1 || entry[m0]->ctr == 0;
    if (m0IsWeak && m0remainsWeak) entry[m0]->utl = 0;
    if (prov == m1) {
      entry[m1]->ctr = i32_supd(entry[m1]->ctr, b.isTaken(), ctrWidth[m1]);
      entry[m1]->utl =
          u32_supd(entry[m1]->utl, prediction == b.isTaken(), utlWidth[m1]);
    }
    if (m0 > 0 && m0IsWeak && predM0 != predM1) {
      meta[metahash()].sumOrSub(predM1 == b.isTaken());
    }
    // If the prediction is wrong but m0 is correct (the prediction was m1),
    // then there is no need to allocate a new entry.
    if (prediction != b.isTaken() && predM0 != b.isTaken()) {
      bool allocated = false;
      for (size_t i = m0 + 1; i < table.size(); ++i) {
        if (entry[i]->utl == 0) {
          entry[i]->tag = tag[i];
          entry[i]->ctr = b.isTaken() ? 0 : -1;
          entry[i]->utl = 1;
          allocated = true;
          break;
        }
      }
      if (!allocated) {
        for (size_t i = m0 + 1; i < table.size(); ++i) {
          entry[i]->utl = u32_ssub(entry[i]->utl, 1);
        }
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
        {"utl_width", utlWidth[i]},
      });
      // clang-format on
    }
    return {
        {"name", "MBPlib Tage"},
        {"tables", tableJson},
    };
  }
};

}  // namespace mbp

#endif  // MBP_EXAMPLES_TAGE_HPP_
