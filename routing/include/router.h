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
  /// @note This function is safe to call multiple times. Although the result
  /// will be the same.
  Result Route();

  explicit Router(Instance);

 private:
  Instance instance_;
  /// @note Is sorted by the start of the interval.
  std::vector<std::tuple<Interval, NetId>> horizontal_constraint_graph_{};
  /// @note The index of the vector is the net id.
  std::vector<std::vector<NetId>> vertical_constraint_graph_{};
  /// @note Inverted VCG for routing in the bottom track.
  std::vector<std::vector<NetId>> inverted_vertical_constraint_graph_{};

  const unsigned number_of_nets_;
  unsigned number_of_routed_nets_ = 0u;
  std::vector<bool> routed_nets_;

  /// @brief Reset all the nets as not routed, so that the routing function can
  /// be called multiple time.
  void ResetRoutedNets_();

  void ConstructHorizontalConstraintGraph_();
  /// @brief Constructs the VCG and the inverted VCG.
  void ConstructVerticalConstraintGraph_();

  std::vector<std::vector<std::tuple<Interval, NetId>>> RouteInTopTracks_();
  std::vector<std::vector<std::tuple<Interval, NetId>>> RouteInBottomTracks_();
  /// @brief Routes all remaining nets in the extra tracks in the channel.
  /// @note Call this function after routing in the top and bottom tracks.
  std::vector<std::vector<std::tuple<Interval, NetId>>> RouteInTracks_();
};

}  // namespace routing

#endif  // ROUTING_ROUTER_H_
