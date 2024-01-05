#ifndef ROUTING_INSTANCE_H_
#define ROUTING_INSTANCE_H_

#include <vector>

namespace routing {

enum BoundaryKind {
  kTop = 0,
  kBottom = 1,
};

using NetId = int;
using NetIds = std::vector<NetId>;
using Interval = std::pair<int, int>;

struct Instance {
  /// @brief The index is mapped to the distance from the innermost boundary.
  /// @example
  /// T2 -----------
  /// T1 -----------
  /// T0 -----------
  ///  (the channel)
  ///  (bottom boundaries)
  std::vector<std::vector<Interval>> top_boundaries;
  /// @brief The index is mapped to the distance from the innermost boundary.
  /// @example
  ///  (top boundaries)
  ///  (the channel)
  /// B0 -----------
  /// B1 -----------
  /// B2 -----------
  std::vector<std::vector<Interval>> bottom_boundaries;
  NetIds top_net_ids;
  NetIds bottom_net_ids;
};

}  // namespace routing

#endif  // ROUTING_INSTANCE_H_
