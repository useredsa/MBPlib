#include "mbp/core/predictor.hpp"
#include "nlohmann/json.hpp"

namespace mbp {

json Predictor::metadata_stats() const { return json::object(); }

json Predictor::execution_stats() const { return json::object(); }

}  // namespace mbp
