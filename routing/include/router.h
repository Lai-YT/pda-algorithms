#ifndef ROUTING_ROUTER_H_
#define ROUTING_ROUTER_H_

#include <tuple>
#include <utility>
#include <vector>

#include "instance.h"
#include "result.h"

namespace routing {

class Router {
 public:
  Result Route();

  explicit Router(Instance instance) : instance_{std::move(instance)} {}

 private:
  Instance instance_;
  /// @note Is sorted by the start of the interval.
  std::vector<std::tuple<Interval, NetId>> horizontal_constraint_graph_{};

  void ConstructHorizontalConstraintGraph_();
  void ConstructVerticalConstraintGraph_();
};

}  // namespace routing

#endif  // ROUTING_ROUTER_H_
