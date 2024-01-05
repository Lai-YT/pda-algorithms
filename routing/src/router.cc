#include "router.h"

#include <algorithm>
#include <cassert>
#include <list>

#include "util.h"

#ifdef DEBUG
#include <iostream>
#endif

using namespace routing;

Result Router::Route() {
  assert(instance_.top_net_ids.size() == instance_.bottom_net_ids.size());
  ConstructHorizontalConstraintGraph_();
  ConstructVerticalConstraintGraph_();

  const auto number_of_nets = NumberOfNets_();
  auto number_of_routed_nets = 0u;
  auto routed
      = std::vector<bool>(number_of_nets + 1 /* index 0 is not used */, false);

  // Since we are not using doglegs, the rectilinear boundaries are only
  // beneficial for those nets that sit exactly in the interval of a distance of
  // boundary. Boundary of a distance may be multiple pieced, boundaries with
  // distance farther are also beneficial.
  auto top_rectilinear_boundaries = std::list<Interval>{};
  /// @note These tracks aren't additional tracks. They use the space of the
  /// boundary. The index is the distance from the innermost boundary.
  auto top_tracks = std::vector<std::vector<std::tuple<Interval, NetId>>>(
      instance_.top_boundaries.size() - 1);
#ifdef DEBUG
  std::cerr << "TOP TRACKS\n";
#endif
  for (auto dist = instance_.top_boundaries.size() - 1;
       dist > 0 /* 0 is the general case */; dist--) {
    // Since the intervals are sorted by the start of the interval, we can do an
    // insertion sort along with the merge to make sure adjacent intervals are
    // treated as one.
    auto it = top_rectilinear_boundaries.begin();
    for (const auto& interval : instance_.top_boundaries.at(dist)) {
      while (true) {
        if (it == top_rectilinear_boundaries.end()) {
          top_rectilinear_boundaries.push_back(interval);
          it = top_rectilinear_boundaries.begin();
          break;
        }
        if (IsAdjacent(interval, *it)) {
          *it = Union(interval, *it);
          break;
        }
        if (interval.second < it->first) {
          // Since we've checked the one before it, interval must be the one
          // right before it.
          top_rectilinear_boundaries.insert(it, interval);
          break;
        }
        // The interval is after it. We need to find the exact place to insert
        // it.
        ++it;
      }
    }
#ifdef DEBUG
    // Routed at dist - 1.
    std::cerr << "Top intervals " << dist << '\t';
    for (const auto& interval : top_rectilinear_boundaries) {
      std::cerr << "(" << interval.first << ", " << interval.second << ") ";
    }
    std::cerr << '\n';
#endif
    auto watermark = -1;
#ifdef DEBUG
    std::cerr << "TOP TRACK " << dist - 1 << '\n';
#endif
    for (const auto& [interval, net_id] : horizontal_constraint_graph_) {
      if (routed.at(net_id)) {
        continue;
      }
      for (const auto& boundary : top_rectilinear_boundaries) {
        if (IsContainedBy(interval, boundary)) {
          if (watermark == -1
              || interval.first > static_cast<unsigned>(watermark)) {
            auto all_parents_routed = true;
            for (auto parent : vertical_constraint_graph_.at(net_id)) {
              if (!routed.at(parent)) {
                all_parents_routed = false;
#ifdef DEBUG
                std::cerr << "Net " << net_id << " has parent " << parent
                          << " not routed\n";
#endif
                break;
              }
            }
            if (all_parents_routed) {
              routed.at(net_id) = true;
              number_of_routed_nets++;
              watermark = interval.second;
              top_tracks.at(dist - 1).emplace_back(interval, net_id);
            }
          }
        }
      }
    }
#ifdef DEBUG
    for (const auto& [interval, net_id] : top_tracks.at(dist - 1)) {
      std::cerr << "(" << interval.first << ", " << interval.second << ")\t"
                << net_id << '\n';
    }
#endif
  }

  // Top boundaries are straightforward, but bottom boundaries are not. The
  // vertical constraint graph has to be inverted, so that we can route the
  // bottom boundaries in the same way as the top boundaries without violating
  // the constraint.

  // On each track in the channel, first set the watermark to -1, then select
  // the net with the smallest* start of interval from the horizontal constraint
  // graph:
  // (1) if the net is not routed and the watermark is less than the start of
  //     the interval:
  //   (a) if all the parents of the net are routed, route the net and set the
  //       watermark to the end of the interval.
  //   (b) if not all the parents of the net are routed, skip the net.
  // (2) if the net is not routed and the watermark is greater than or equal to
  //     the start of the interval, skip the net.
  // (3) if the net is routed, skip the net.
  //  If there's not more nets that are possible to be routed in this track, go
  //  to the next track.
  // * From those nets that are not skipped.

  // On each track, several nets may be routed.
  auto tracks = std::vector<std::vector<std::tuple<Interval, NetId>>>{};
#ifdef DEBUG
  std::cerr << "TRACKS\n";
#endif
  while (number_of_routed_nets < number_of_nets) {
    assert(tracks.size() < number_of_nets
        && "the worst routing result shall not have to use more tracks than the number of nets");
    tracks.emplace_back();
    auto watermark = -1;
#ifdef DEBUG
    std::cerr << "TRACK " << tracks.size() << '\n';
#endif
    for (const auto& [interval, net_id] : horizontal_constraint_graph_) {
      if (routed.at(net_id)) {
        continue;
      }
      if (watermark == -1
          || interval.first > static_cast<unsigned>(watermark)) {
        auto all_parents_routed = true;
        for (auto parent : vertical_constraint_graph_.at(net_id)) {
          if (!routed.at(parent)) {
            all_parents_routed = false;
#ifdef DEBUG
            std::cerr << "Net " << net_id << " has parent " << parent
                      << " not routed\n";
#endif
            break;
          }
        }
        if (all_parents_routed) {
          routed.at(net_id) = true;
          number_of_routed_nets++;
          watermark = interval.second;
          tracks.back().emplace_back(interval, net_id);
        }
      }
    }
#ifdef DEBUG
    for (const auto& [interval, net_id] : tracks.back()) {
      std::cerr << "(" << interval.first << ", " << interval.second << ")\t"
                << net_id << '\n';
    }
#endif
  }
  return Result{
      .top_tracks = top_tracks,
      .tracks = tracks,
      .bottom_tracks = {},
  };
}

void Router::ConstructHorizontalConstraintGraph_() {
  // The horizontal constraint regardless of whether the net is at the top or
  // the bottom. For each net id, find its smallest and largest index in the mix
  // top and bottom boundaries.

  const auto number_of_nets = NumberOfNets_();
  auto interval_of_nets
      = std::vector<Interval>(number_of_nets + 1 /* index 0 is not used */);
  std::fill(interval_of_nets.begin(), interval_of_nets.end(),
            Interval{instance_.top_net_ids.size() - 1, 0});
  for (auto i = std::size_t{0}, e = instance_.top_net_ids.size(); i < e; i++) {
    {
      auto top_net_id = instance_.top_net_ids.at(i);
      auto& interval = interval_of_nets.at(top_net_id);
      interval.first = std::min(interval.first, i);
      interval.second = std::max(interval.second, i);
    }
    {
      auto bottom_net_id = instance_.bottom_net_ids.at(i);
      auto& interval = interval_of_nets.at(bottom_net_id);
      interval.first = std::min(interval.first, i);
      interval.second = std::max(interval.second, i);
    }
  }
  // Sort the intervals by the start of the interval.
  // 0 is skipped. It's fine that we've take 0 into account in the previous
  // step.
  for (auto net_id = 1u; net_id <= number_of_nets; net_id++) {
    horizontal_constraint_graph_.emplace_back(interval_of_nets.at(net_id),
                                              net_id);
  }
  std::sort(horizontal_constraint_graph_.begin(),
            horizontal_constraint_graph_.end(),
            [](const auto& lhs, const auto& rhs) {
              return std::get<0>(lhs).first < std::get<0>(rhs).first;
            });
#ifdef DEBUG
  std::cerr << "HORIZONTAL CONSTRAINT GRAPH\n";
  for (const auto& [interval, net_id] : horizontal_constraint_graph_) {
    std::cerr << "(" << interval.first << ", " << interval.second << ")\t"
              << net_id << '\n';
  }
#endif
}

void Router::ConstructVerticalConstraintGraph_() {
  // For each net, we have a list keep its parents. Let n be the net at index i
  // of the bottom boundary, m be the net at index i of the top boundary. If n
  // != m, then m is a parent of n.

  auto number_of_nets = NumberOfNets_();
  vertical_constraint_graph_.resize(number_of_nets
                                    + 1 /* index 0 is not used */);
  for (auto i = std::size_t{0}, e = instance_.top_net_ids.size(); i < e; i++) {
    auto top_net_id = instance_.top_net_ids.at(i);
    auto bottom_net_id = instance_.bottom_net_ids.at(i);
    if (top_net_id == kEmptySlot || bottom_net_id == kEmptySlot) {
      continue;
    }
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
  std::cerr << "VERTICAL CONSTRAINT GRAPH\n";
  for (auto net_id = 1u; net_id <= number_of_nets; net_id++) {
    std::cerr << net_id << ": ";
    for (auto parent : vertical_constraint_graph_.at(net_id)) {
      std::cerr << parent << " ";
    }
    std::cerr << '\n';
  }
#endif
}

unsigned Router::NumberOfNets_() const {
  // The id of the nets are guaranteed to be positive (0 is not a net id) and
  // consecutive. Thus, the largest net id is the number of nets.
  return std::max(*std::max_element(instance_.top_net_ids.begin(),
                                    instance_.top_net_ids.end()),
                  *std::max_element(instance_.bottom_net_ids.begin(),
                                    instance_.bottom_net_ids.end()));
}
