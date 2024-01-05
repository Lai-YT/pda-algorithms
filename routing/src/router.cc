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
  // For each net, we have a list keep its parents. Let n be the net at index i
  // of the bottom boundary, m be the net at index i of the top boundary. If n
  // != m, then m is a parent of n.

  // The id of the nets are guaranteed to be positive (0 is not a net id) and
  // consecutive. Thus, the largest net id is the number of nets.
  auto number_of_nets
      = std::max(*std::max_element(instance_.top_net_ids.begin(),
                                   instance_.top_net_ids.end()),
                 *std::max_element(instance_.bottom_net_ids.begin(),
                                   instance_.bottom_net_ids.end()));
  vertical_constraint_graph_.resize(number_of_nets
                                    + 1 /* index 0 is not used */);
  for (auto i = std::size_t{0}, e = instance_.top_net_ids.size(); i < e; i++) {
    auto top_net_id = instance_.top_net_ids.at(i);
    auto bottom_net_id = instance_.bottom_net_ids.at(i);
    if (top_net_id != bottom_net_id) {
      // NOTE: This approach of avoiding duplicates may be inefficient, but the
      // number of parents is small, so it should be fine.
      auto in_list = false;
      for (auto parent : vertical_constraint_graph_.at(bottom_net_id)) {
        if (parent == top_net_id) {
          in_list = true;
          break;
        }
      }
      if (!in_list) {
        vertical_constraint_graph_.at(bottom_net_id).push_back(top_net_id);
      }
    }
  }
#ifdef DEBUG
  std::cout << "VERTICAL CONSTRAINT GRAPH\n";
  for (auto i = std::size_t{0}, e = vertical_constraint_graph_.size(); i < e;
       i++) {
    std::cout << i << ": ";
    for (auto net_id : vertical_constraint_graph_.at(i)) {
      std::cout << net_id << " ";
    }
    std::cout << '\n';
  }
#endif
}
