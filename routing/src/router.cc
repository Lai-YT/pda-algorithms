#include "router.h"

#include <algorithm>
#include <cassert>
#include <unordered_map>

#ifdef DEBUG
#include <iostream>
#endif

using namespace routing;

Result Router::Route() {
  assert(instance_.top_net_ids.size() == instance_.bottom_net_ids.size());
  ConstructHorizontalConstraintGraph_();
  ConstructVerticalConstraintGraph_();
  return Result{};
}

void Router::ConstructHorizontalConstraintGraph_() {
  // The horizontal constraint regardless of whether the net is at the top or
  // the bottom. For each net id, find its smallest and largest index in the mix
  // top and bottom boundaries.
  // TODO: The id of the nets are guaranteed to be positive (0 is not a net id)
  // and consecutive. Thus, we can use a vector instead of a map.
  auto interval_map = std::unordered_map<NetId, Interval>{};
  for (auto i = std::size_t{0}, e = instance_.top_net_ids.size(); i < e; i++) {
    {
      auto top_net_id = instance_.top_net_ids.at(i);
      if (!interval_map.contains(top_net_id)) {
        interval_map[top_net_id] = {e - 1, 0};
      }
      auto& interval = interval_map[top_net_id];
      interval.first = std::min(interval.first, i);
      interval.second = std::max(interval.second, i);
    }
    {
      auto bottom_net_id = instance_.bottom_net_ids.at(i);
      if (!interval_map.contains(bottom_net_id)) {
        interval_map[bottom_net_id] = {e - 1, 0};
      }
      auto& interval = interval_map[bottom_net_id];
      interval.first = std::min(interval.first, i);
      interval.second = std::max(interval.second, i);
    }
  }
  // Sort the intervals by the start of the interval.
  for (const auto& [net_id, interval] : interval_map) {
    horizontal_constraint_graph_.emplace_back(interval, net_id);
  }
  std::sort(horizontal_constraint_graph_.begin(),
            horizontal_constraint_graph_.end(),
            [](const auto& lhs, const auto& rhs) {
              return std::get<0>(lhs).first < std::get<0>(rhs).first;
            });
#ifdef DEBUG
  std::cout << "HORIZONTAL CONSTRAINT GRAPH\n";
  for (const auto& [interval, net_id] : horizontal_constraint_graph_) {
    std::cout << "(" << interval.first << ", " << interval.second << ")\t"
              << net_id << '\n';
  }
#endif
}

void Router::ConstructVerticalConstraintGraph_() {
  // TODO
}
