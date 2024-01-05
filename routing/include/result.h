#ifndef ROUTING_RESULT_H_
#define ROUTING_RESULT_H_

#include <tuple>
#include <vector>

#include "instance.h"

namespace routing {

struct Result {
  /// @brief The space between the top rectilinear boundary and the topmost
  /// track.
  std::vector<std::vector<std::tuple<Interval, NetId>>> top_tracks;
  /// @brief The extra tracks in the channel.
  std::vector<std::vector<std::tuple<Interval, NetId>>> tracks;
  /// @brief The space between the bottom rectilinear boundary and the
  /// bottommost track.
  std::vector<std::vector<std::tuple<Interval, NetId>>> bottom_tracks;
};

}  // namespace routing

#endif  // ROUTING_RESULT_H_
