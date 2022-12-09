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

  using Entries = std::vector<Entry*>;
  using Tags = std::vector<uint32_t>;

  static size_t LastMatch(const Entries& entry, const Tags& tag) {
    // Provider component.
    size_t i = entry.size() - 1;
    while (i > 0 && entry[i]->tag != tag[i]) --i;
    return i;
  }

  // TODO(useredsa): Include metapredictor for tage
  // static std::pair<int, int> LastTwoMatches(const Entries& entry,
  //                                           const Tags& tag) {
  //   // Provider component.
  //   int p = entry.size() - 1;
  //   while (p > 0 && entry[p]->tag != tag[p]) --p;
  //   // Alternate component.
  //   int alt = p - 1;
  //   while (alt > 0 && entry[alt]->tag != tag[alt]) alt--;
  //   return {p, alt};
  // }

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
  // Saved Values
  Entries entry;
  Tags tag;
  uint64_t entriesIp;
  size_t prov;
  bool prediction;
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
        prov(0),
        prediction(false),
        updatedGhist(false) {
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

  bool predict(uint64_t ip) override {
    if (entriesIp == ip && updatedGhist == false) return prediction;
    entriesIp = ip;
    updatedGhist = false;

    computeEntriesAndTags(ip);
    prov = LastMatch(entry, tag);
    return prediction = entry[prov]->ctr >= 0;
  }

  void train(const Branch& b) override {
    predict(b.ip());

    entry[prov]->ctr = i32_supd(entry[prov]->ctr, b.isTaken(), ctrWidth[prov]);
    entry[prov]->utl =
        u32_supd(entry[prov]->utl, prediction == b.isTaken(), utlWidth[prov]);
    if (prediction != b.isTaken()) {
      bool allocated = false;
      for (size_t i = prov + 1; i < table.size(); ++i) {
        if (entry[i]->utl == 0) {
          entry[i]->tag = tag[i];
          entry[i]->ctr = b.isTaken() ? 0 : -1;
          entry[i]->utl = 1;
          allocated = true;
          break;
        }
      }
      if (!allocated) {
        for (size_t i = prov + 1; i < table.size(); ++i) {
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
