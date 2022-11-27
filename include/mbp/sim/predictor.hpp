#ifndef MBP_PREDICTOR_HPP_
#define MBP_PREDICTOR_HPP_

#include <cstdint>

#include "nlohmann/json_fwd.hpp"

#include "mbp/sim/branch.hpp"

namespace mbp {

using json = nlohmann::ordered_json;

/**
 * MBPlib Predictor Interface.
 */
class Predictor {
 public:
  /**
   * Outcome prediction for a given program address.
   *
   * The function shall not modify the state of the predictor
   * in a way that would affect future predictions.
   * The function is not const to allow the caching of computed hashes,
   * since `update`, which is usually called after `predict`,
   * more than likely will need to recompute those hashes.
   */
  virtual bool predict(uint64_t ip) = 0;

  /**
   * Update to predict better the given branch.
   */
  virtual void train(const Branch& branch) = 0;

  /**
   * Update to predict better any branch based on this branch information.
   */
  virtual void track(const Branch& branch) = 0;

  /**
   * Gets a json object with metadata stats.
   */
  virtual json metadata_stats() const;

  /**
   * Gets a json object with execution stats.
   */
  virtual json execution_stats() const;

  /**
   * Resets all execution statistics to their default value.
   */
  virtual void clear_execution_stats() {}
};

}  // namespace mbp

#endif  // MBP_PREDICTOR_HPP_
