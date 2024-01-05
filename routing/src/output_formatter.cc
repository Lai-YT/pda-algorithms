#include "output_formatter.h"

#include <cassert>
#include <ostream>
#include <tuple>
#include <unordered_map>

#include "instance.h"

using namespace routing;

enum class RoutePlaceKind { kTop, kTrack, kBottom };

void OutputFormatter::Out() {
  // The number of extra tracks in the channel.
  out_ << "Channel density: " << result_.tracks.size() << '\n';

  // What we get from the result is the mapping from the track to the net, while
  // we need the mapping from the net to the track for the output.
  auto route_pos_of_nets = std::unordered_map<
      NetId,
      std::tuple<RoutePlaceKind, std::size_t /* track number */, Interval>>{};
  for (auto i = std::size_t{0}, e = result_.top_tracks.size(); i < e; ++i) {
    for (auto [interval, net_id] : result_.top_tracks.at(i)) {
      route_pos_of_nets[net_id] = {RoutePlaceKind::kTop, i, interval};
    }
  }
  for (auto i = std::size_t{0}, e = result_.tracks.size(); i < e; ++i) {
    for (auto [interval, net_id] : result_.tracks.at(i)) {
      // Although we route from the top to the bottom, the bottom most track
      // is 1.
      route_pos_of_nets[net_id] = {RoutePlaceKind::kTrack, e - i, interval};
    }
  }
  for (auto i = std::size_t{0}, e = result_.bottom_tracks.size(); i < e; ++i) {
    for (auto [interval, net_id] : result_.bottom_tracks.at(i)) {
      route_pos_of_nets[net_id] = {RoutePlaceKind::kBottom, i, interval};
    }
  }

  // Where each net is routed. In the order of the nets.
  auto number_of_net = route_pos_of_nets.size();
  for (auto i = 1u; i <= number_of_net; ++i) {
    out_ << "Net " << i << '\n';
    auto [route_type, track_number, interval] = route_pos_of_nets.at(i);
    auto route_place_abbr = '\0';
    switch (route_type) {
      case RoutePlaceKind::kTop:
        route_place_abbr = 'T';
        break;
      case RoutePlaceKind::kTrack:
        route_place_abbr = 'C';
        break;
      case RoutePlaceKind::kBottom:
        route_place_abbr = 'B';
        break;
      default:
        assert(false && "Unknown kind of route place");
        break;
    }
    out_ << route_place_abbr << track_number << ' ' << interval.first << ' '
         << interval.second;
    if (i != number_of_net) {
      out_ << '\n';
      // No end-of-file newline.
    }
  }
}
