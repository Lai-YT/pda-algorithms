#include "path_finder.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include "circuit.h"
#include "mos.h"

#ifdef DEBUG
#include <string>
#endif
#ifndef NDEBUG
#include <numeric>
#endif

using namespace euler;

namespace {

//
// NOTE: One may say that the following functions should be member functions of
// the PathFinder, and the parameters should be the data members. However, these
// functions doesn't depend on the data members of the PathFinder, so they are
// not member functions.
//

bool IsNeighbor(const Vertex& a, const Vertex& b);
/// @note To be neighbor, the two MOS must have their source or drain
/// connected. The gate is used for pairing and the substrate usually connects
/// to the same point, so we don't depend on them.
bool IsNeighbor(const Mos& a, const Mos& b);

/// @note In case of multiply connections, we choose one in unspecified order.
Edge FindEdgeToNeighbor(const Vertex& a, const Vertex& b);
std::shared_ptr<Net> FindEdgeToNeighbor(const Mos& a, const Mos& b);

/// @brief To connect two paths, we add a dummy to end of the first path, and a
/// dummy to the start of the second path. These 2 dummies are then connected
/// with a dummy net.
/// @details The order of Nets in the path corresponds to the sequence of
/// connections. For a MOS, which typically has 4 pins, two of these pins are
/// commonly connected to the same point. As a result, the standard order is
/// (left, gate, right). Notably, there are only 3 connections in this order.
/// However, if a MOS has all 4 pins connected to different points, and it acts
/// as either the starting or ending point of the path, we choose one pin to
/// exclude.
HamiltonPath ConnectHamiltonPathOfSubgraphsWithDummy(
    const std::vector<HamiltonPath>&);

/// @brief If the length of the Hamilton path is 1, nets other than the gate are
/// free. In this case, there are 2 free edges, one of them is returned.
/// Otherwise, the free net is the one that is not used as a connection between
/// the next vertex and the gate.
Edge FindFreeNetOfStartingVertex(const HamiltonPath&);

/// @brief If the length of the Hamilton path is 1, nets other than the gate are
/// free. In this case, there are 2 free edges, one of them is returned.
/// Otherwise, the free net is the one that is not used as a connection between
/// the previous vertex and the gate.
Edge FindFreeNetOfEndingVertex(const HamiltonPath&);

/// @return The nets that connect the MOS in the Hamilton path, including the
/// gate connections of the MOS.
std::vector<Edge> GetEdgesOf(const HamiltonPath&);

/// @note For HPWL calculation, we need to know the Hamilton distance between
/// true nets. This makes the existence of gate a noise. So we exclude the gate.
std::vector<Edge> GetEdgesWithGateExcludedOf(const HamiltonPath& path);

std::vector<std::shared_ptr<Net>> NetsOf(const Mos&);

}  // namespace

std::tuple<HamiltonPath, std::vector<Edge>, double> PathFinder::FindPath() {
  GroupVertices_();
  BuildGraph_();

#ifdef DEBUG
  std::cerr << "=== Graph ===" << std::endl;
  for (const auto& vertex : vertices_) {
    std::cerr << vertex.first->GetName() << " " << vertex.second->GetName()
              << std::endl;
    for (const auto& neighbor : adjacency_list_.at(vertex)) {
      std::cerr << "  " << neighbor.first->GetName() << " "
                << neighbor.second->GetName() << std::endl;
    }
  }
#endif

  auto paths = FindHamiltonPaths_();

#ifdef DEBUG
  std::cerr << "=== Paths ===" << std::endl;
  for (const auto& path : paths) {
    for (const auto& [p, n] : path) {
      std::cerr << p->GetName() << "\t" << n->GetName() << std::endl;
    }
    std::cerr << "@@@" << std::endl;
  }
#endif

  auto path = ConnectHamiltonPathOfSubgraphsWithDummy(paths);
  return {path, GetEdgesOf(path), CalculateHpwl_(path)};
}

void PathFinder::GroupVertices_() {
  // Separate the P MOS transistors from the N MOS transistors.
  auto p_mos = std::map<std::shared_ptr<Net> /* gate */,
                        std::vector<std::shared_ptr<Mos>>>{};
  auto n_mos = std::map<std::shared_ptr<Net> /* gate */,
                        std::vector<std::shared_ptr<Mos>>>{};
  for (const auto& mos : circuit_->mos) {
    if (mos->GetType() == Mos::Type::kP) {
      p_mos[mos->GetGate()].push_back(mos);
    } else {
      n_mos[mos->GetGate()].push_back(mos);
    }
  }

  // Group the P MOS transistors with the N MOS transistors.
  for (const auto& [gate, p_mos_with_same_gate] : p_mos) {
    assert(n_mos.find(gate) != n_mos.end()
           && "A P MOS should at least have a corresponding N MOS.");
    const auto& n_mos_with_same_gate = n_mos.at(gate);
    // If there is only one P MOS transistor, then it is paired with the
    // corresponding N MOS transistor.
    if (p_mos_with_same_gate.size() == 1) {
      assert(n_mos_with_same_gate.size() == 1
             && "An N MOS should at least have a corresponding P MOS.");
      vertices_.emplace_back(p_mos_with_same_gate.front(),
                             n_mos_with_same_gate.front());
      continue;
    }

    auto remaining_n_mos = n_mos_with_same_gate;
    auto remaining_p_mos = p_mos_with_same_gate;
    // If the P MOS transistor and the N MOS transistor share another
    // common connection, then they are paired.
    for (const auto& n : n_mos_with_same_gate) {
      for (const auto& p : p_mos_with_same_gate) {
        // NOTE: The connection of substrate doesn't count since all P MOS
        // usually all connect their substrate to the same point. So are the N
        // MOS.
        if (p->GetDrain() == n->GetDrain()
            || p->GetSource() == n->GetSource()) {
          vertices_.emplace_back(p, n);
          remaining_p_mos.erase(
              std::find_if(remaining_p_mos.begin(), remaining_p_mos.end(),
                           [&p](const auto& p_) { return p_ == p; }));
          remaining_n_mos.erase(
              std::find_if(remaining_n_mos.begin(), remaining_n_mos.end(),
                           [&n](const auto& n_) { return n_ == n; }));
          break;
        }
      }
    }

#ifdef DEBUG
    for (const auto& p : remaining_p_mos) {
      std::cerr << "Remaining P MOS: " << p->GetName() << std::endl;
    }
    for (const auto& n : remaining_n_mos) {
      std::cerr << "Remaining N MOS: " << n->GetName() << std::endl;
    }
#endif

    // While sometime multiple P and N MOS share only the gate. In such case, we
    // can pair them in any way.
    assert(remaining_n_mos.size() == remaining_p_mos.size());
    for (auto i = std::size_t{0}; i < remaining_n_mos.size(); i++) {
      vertices_.emplace_back(remaining_p_mos.at(i), remaining_n_mos.at(i));
    }
  }

#ifdef DEBUG
  std::cerr << "=== MOS pairs ===" << std::endl;
  for (const auto& [p, n] : vertices_) {
    std::cerr << p->GetName() << "\t" << n->GetName() << std::endl;
  }
#endif
}

void PathFinder::BuildGraph_() {
  // Each pair is a vertex in the graph. Two vertex are neighbors if they have
  // their P MOS connected and N MOS connected.
  for (const auto& v : vertices_) {
    adjacency_list_[v] = Neighbors{};
    for (const auto& v_ : vertices_) {
      if (v == v_) {
        continue;
      }
      if (IsNeighbor(v, v_)) {
        adjacency_list_.at(v).push_back(v_);
      }
    }
  }
}

std::vector<HamiltonPath> PathFinder::FindHamiltonPaths_() {
  // Select from the to visited list should be faster than iterating through all
  // the vertices and checking whether they are in the visited list.
  auto to_visit = std::set<Vertex>{vertices_.cbegin(), vertices_.cend()};
  auto paths = std::vector<HamiltonPath>{};
  while (!to_visit.empty()) {
    // Randomly select a vertex to start. We select the first one for
    // simplicity.
    auto path = HamiltonPath{*to_visit.begin()};
    to_visit.erase(to_visit.begin());

    // Find a Hamilton path.
    while (true) {
      if (auto extended_path = Extend_(path, to_visit); extended_path) {
        path = std::move(*extended_path);
        continue;
      }

      // Can no longer extend. Try to rotate the path.
      auto found = false;
      for (const auto& rotated_path : Rotate_(path)) {
        if (auto extended_path = Extend_(rotated_path, to_visit);
            extended_path) {
          path = std::move(*extended_path);
          found = true;
          break;
        }
      }
      if (found) {
        continue;
      }

      // Cannot extend the path even after rotating. This path is done.
      paths.push_back(std::move(path));
      break;
    }
  }
  return paths;
}

std::optional<HamiltonPath> PathFinder::Extend_(
    HamiltonPath path, std::set<Vertex>& to_visit) const {
  // If the neighbor of the start or end vertex is not in the path, then we add
  // it into the path. We check the end vertex first, due to the efficiency of
  // inserting to the back over the front.
  // NOTE: If a net is already used in a connection, we cannot uses it
  // again.
  for (const auto& neighbor : adjacency_list_.at(path.back())) {
#ifdef DEBUG
    std::cerr << "End vertex: " << path.back().first->GetName() << "\t"
              << path.back().second->GetName() << std::endl;
    std::cerr << "Neighbor: " << neighbor.first->GetName() << "\t"
              << neighbor.second->GetName() << std::endl;
    auto edge_to_neighbor = FindEdgeToNeighbor(path.back(), neighbor);
    auto free_net_of_ending_vertex = FindFreeNetOfEndingVertex(path);
    std::cerr << "Free net of ending vertex: "
              << free_net_of_ending_vertex.first->GetName() << "\t"
              << free_net_of_ending_vertex.second->GetName() << std::endl;
    std::cerr << "Edge to neighbor: " << edge_to_neighbor.first->GetName()
              << "\t" << edge_to_neighbor.second->GetName() << std::endl;
#endif

    if (to_visit.find(neighbor) != to_visit.cend()
        && FindEdgeToNeighbor(path.back(), neighbor)
               == FindFreeNetOfEndingVertex(path)) {
      path.push_back(neighbor);
      to_visit.erase(neighbor);
      return path;
    }
  }
  for (const auto& neighbor : adjacency_list_.at(path.front())) {
#ifdef DEBUG
    std::cerr << "Start vertex: " << path.front().first->GetName() << "\t"
              << path.front().second->GetName() << std::endl;
    std::cerr << "Neighbor: " << neighbor.first->GetName() << "\t"
              << neighbor.second->GetName() << std::endl;
    auto edge_to_neighbor = FindEdgeToNeighbor(path.front(), neighbor);
    auto free_net_of_starting_vertex = FindFreeNetOfStartingVertex(path);
    std::cerr << "Free net of starting vertex: "
              << free_net_of_starting_vertex.first->GetName() << "\t"
              << free_net_of_starting_vertex.second->GetName() << std::endl;
#endif
    if (to_visit.find(neighbor) != to_visit.cend()
        && FindEdgeToNeighbor(path.front(), neighbor)
               == FindFreeNetOfStartingVertex(path)) {
      path.insert(path.cbegin(), neighbor);
      to_visit.erase(neighbor);
      return path;
    }
  }
  return std::nullopt;
}

std::vector<HamiltonPath> PathFinder::Rotate_(const HamiltonPath& path) const {
  if (path.size() <= 2) {
    return {};
  }

#ifdef DEBUG
  std::cerr << "=== Rotating ===" << std::endl;
  std::cerr << "original: " << std::endl;
  for (const auto& [p, n] : path) {
    std::cerr << p->GetName() << "\t" << n->GetName() << std::endl;
  }
#endif

  auto rotated_paths = std::vector<HamiltonPath>{};
  // If the start or end vertex has a short cut to the vertex in the middle of
  // the path, that vertex becomes the new end or start vertex, respectively.
  // NOTE: The rotation is actually a reverse.
  for (auto i = std::size_t{2} /* skip the immediate neighbor */;
       i < path.size(); i++) {
    if (IsNeighbor(path.front(), path.at(i))) {
      // Make a copy for the rotating.
      auto rotated_path = path;
      std::reverse(rotated_path.begin(), rotated_path.begin() + i);
      rotated_paths.push_back(std::move(rotated_path));
    }
  }
  for (auto i = std::size_t{0};
       i < path.size() - 2 /* skip the immediate neighbor */; i++) {
    if (IsNeighbor(path.back(), path.at(i))) {
      auto rotated_path = path;
      // The path is broke from the left neighbor of the vertex at i.
      std::reverse(rotated_path.begin() + i + 1, rotated_path.end());
      rotated_paths.push_back(std::move(rotated_path));
    }
  }
  return rotated_paths;
}

double PathFinder::CalculateHpwl_(const HamiltonPath& path) const {
  // Design rule parameters.
  constexpr auto kVerticalWidthIncrement = 27.0;
  constexpr auto kHorizontalExtension = 25.0;
  constexpr auto kGateSpacing = 34.0;
  constexpr auto kHorizontalGateWidth = 20.0;
  constexpr auto kUnitHorizontalWidth = kGateSpacing + kHorizontalGateWidth;

  auto net_order = GetEdgesWithGateExcludedOf(path);
  auto nets = std::vector<std::shared_ptr<Net>>{};
  for (const auto& [_, net] : circuit_->nets) {
    nets.push_back(net);
  }
  auto hpwl = 0.0;
  // For each net (excluding dummy nets not connected to any MOS other than the
  // dummy MOS), we determine the maximum wire length in the path of the P-type
  // MOS and the N-type MOS. If both types contain such paths connected by this
  // net, we add a vertical length, as the net has to cross both types of MOS.
  // If only one type of MOS contains such a path, but the other type has the
  // net at a single point, we also need to add a vertical length.
  // If both types have the net at a single point, we don't need to calculate
  // the wire length by their Hamilton distance.
  // NOTE: The width of the MOS are said to be consistent among the
  // same type and the length are all the same. So we can just use the first
  // one.
  const auto width_of_p_mos = path.front().first->GetWidth();
  const auto width_of_n_mos = path.front().second->GetWidth();
  [[maybe_unused]] const auto length_of_mos = path.front().first->GetLength();
  const auto vertical_wire_length
      = kVerticalWidthIncrement + (width_of_p_mos + width_of_n_mos) / 2;
  for (auto net : nets) {
    /// @note The indices are sorted.
    auto idx_in_p = std::vector<std::size_t>{};
    /// @note The indices are sorted.
    auto idx_in_n = std::vector<std::size_t>{};
    for (auto i = std::size_t{0}; i < net_order.size(); i++) {
      if (net_order.at(i).first == net) {
        idx_in_p.push_back(i);
      }
      if (net_order.at(i).second == net) {
        idx_in_n.push_back(i);
      }
    }

#ifdef DEBUG
    std::cerr << "=== Idx of " << net->GetName() << " ===" << std::endl;
    std::cerr << "P MOS: ";
    for (const auto& idx : idx_in_p) {
      std::cerr << idx << " ";
    }
    std::cerr << std::endl;
    std::cerr << "N MOS: ";
    for (const auto& idx : idx_in_n) {
      std::cerr << idx << " ";
    }
    std::cerr << std::endl;
#endif

    const auto HorizontalWidthOf
        = [kUnitHorizontalWidth](
              const std::vector<std::size_t>& sorted_idx_of_nets) {
            return kUnitHorizontalWidth
                   * (sorted_idx_of_nets.back() - sorted_idx_of_nets.front());
          };

    const auto IsCoveringTheEnd
        = [&net_order](const std::vector<std::size_t>& sorted_idx_of_nets) {
            return sorted_idx_of_nets.back() == net_order.size() - 1;
          };

    const auto IsCoveringTheStart
        = [](const std::vector<std::size_t>& sorted_idx_of_nets) {
            return sorted_idx_of_nets.front() == 0;
          };

    // If any of the net is at the end of the path, we need to use the
    // extension width instead of a normal gate spacing.
    auto adjustment = 0.0;
    // (1) both P and N MOS have the net at a single point.
    if (idx_in_p.size() == 1 && idx_in_n.size() == 1) {
      // Treat them as in the same type + vertical wire length.
      /// @note The indices are sorted.
      auto augmented_idx
          = decltype(idx_in_p){std::min(idx_in_p.front(), idx_in_n.front()),
                               std::max(idx_in_p.front(), idx_in_n.front())};
      hpwl += HorizontalWidthOf(augmented_idx) + vertical_wire_length;
      adjustment
          = IsCoveringTheEnd(augmented_idx) + IsCoveringTheStart(augmented_idx);
    } else if (idx_in_p.size() > 1 && idx_in_n.size() == 1) {
      // (2) P MOS has the net at multiple points, but N MOS has the net at a
      // single point. Treat them as all in the P type + vertical wire length.
      auto augmented_idx_in_p = idx_in_p;
      augmented_idx_in_p.push_back(idx_in_n.front());
      hpwl += HorizontalWidthOf(augmented_idx_in_p) + vertical_wire_length;
      adjustment = IsCoveringTheEnd(augmented_idx_in_p)
                   + IsCoveringTheStart(augmented_idx_in_p);
    } else if (idx_in_p.size() == 1 && idx_in_n.size() > 1) {
      // (3) P MOS has the net at a single point, but N MOS has the net at
      // multiple points. Treat them as all in the N type + vertical wire
      // length.
      auto augmented_idx_in_n = idx_in_n;
      augmented_idx_in_n.push_back(idx_in_p.front());
      hpwl += HorizontalWidthOf(augmented_idx_in_n) + vertical_wire_length;
      adjustment = IsCoveringTheEnd(augmented_idx_in_n)
                   + IsCoveringTheStart(augmented_idx_in_n);
    } else if (idx_in_p.size() > 1 && idx_in_n.size() > 1) {
      // (4) Both P and N MOS have the net at multiple points. If these two path
      // do overlap, we add up the wire length of the two paths + vertical wire
      // length. If these two path do not overlap, we add up the wire length of
      // the two paths + vertical wire length + wire length between the two
      // paths.
      hpwl += HorizontalWidthOf(idx_in_p) + HorizontalWidthOf(idx_in_n)
              + vertical_wire_length;
      if (idx_in_p.front() > idx_in_n.back()) {
        // Path in P type is after the path in N type.
        hpwl += kUnitHorizontalWidth * (idx_in_p.front() - idx_in_n.back());
      } else if (idx_in_n.front() > idx_in_p.back()) {
        // Path in N type is after the path in P type.
        hpwl += kUnitHorizontalWidth * (idx_in_n.front() - idx_in_p.back());
      }
      adjustment = IsCoveringTheEnd(idx_in_p) + IsCoveringTheStart(idx_in_p)
                   + IsCoveringTheEnd(idx_in_n) + IsCoveringTheStart(idx_in_n);
    } else if (idx_in_p.size() > 1 && idx_in_n.empty()) {
      // (5) Only P MOS has the net at multiple points. No vertical wire length.
      hpwl += HorizontalWidthOf(idx_in_p);
      adjustment = IsCoveringTheEnd(idx_in_p) + IsCoveringTheStart(idx_in_p);
    } else if (idx_in_p.empty() && idx_in_n.size() > 1) {
      // (6) Only N MOS has the net at multiple points. No vertical wire length.
      hpwl += HorizontalWidthOf(idx_in_n);
      adjustment = IsCoveringTheEnd(idx_in_n) + IsCoveringTheStart(idx_in_n);
    } else {
      // Single point in a type. No wire length.
      continue;
    }
    hpwl += (-kGateSpacing + kHorizontalExtension) / 2.0 * adjustment;

#ifdef DEBUG
    std::cerr << "HPWL: " << hpwl << std::endl;
#endif
  }
  return hpwl;
}

namespace {

bool IsNeighbor(const Vertex& a, const Vertex& b) {
  return IsNeighbor(*a.first, *b.first) && IsNeighbor(*a.second, *b.second);
}

bool IsNeighbor(const Mos& a, const Mos& b) {
  return a.GetDrain() == b.GetDrain() || a.GetSource() == b.GetSource()
         || a.GetDrain() == b.GetSource() || a.GetSource() == b.GetDrain();
}

Edge FindEdgeToNeighbor(const Vertex& a, const Vertex& b) {
  return {FindEdgeToNeighbor(*a.first, *b.first),
          FindEdgeToNeighbor(*a.second, *b.second)};
}

std::shared_ptr<Net> FindEdgeToNeighbor(const Mos& a, const Mos& b) {
  if (!IsNeighbor(a, b)) {
    return nullptr;
  }
  if (a.GetDrain() == b.GetDrain() || a.GetDrain() == b.GetSource()) {
    return a.GetDrain();
  }
  if (a.GetSource() == b.GetSource() || a.GetSource() == b.GetDrain()) {
    return a.GetSource();
  }
  assert(false && "Cannot find the net to the neighbor.");
  return nullptr;
}

HamiltonPath ConnectHamiltonPathOfSubgraphsWithDummy(
    const std::vector<HamiltonPath>& paths) {
  if (paths.size() == 1) {
    return paths.front();
  }
  auto path = HamiltonPath{paths.front()};
  for (auto i = std::size_t{1}; i < paths.size(); i++) {
    // Get the net that is free (not already used as an edge) to be connected
    // with the dummy.
    // The 2 dummies are connected with the dummy net.
    auto dummy_net = std::make_shared<Net>("Dummy");
    auto ending_vertex = path.back();
    auto ending_free_net = FindFreeNetOfEndingVertex(path);
    // The size of the dummy is the same as the MOS next to it.
    auto ending_dummy = Vertex{
        Mos::Create("Dummy", Mos::Type::kP, ending_free_net.first, dummy_net,
                    dummy_net, dummy_net, ending_vertex.first->GetWidth(),
                    ending_vertex.first->GetLength()),
        Mos::Create("Dummy", Mos::Type::kN, ending_free_net.second, dummy_net,
                    dummy_net, dummy_net, ending_vertex.second->GetWidth(),
                    ending_vertex.second->GetLength())};
    auto starting_vertex = paths.at(i).front();
    auto starting_free_net = FindFreeNetOfStartingVertex(paths.at(i));
    auto starting_dummy = Vertex{
        Mos::Create("Dummy", Mos::Type::kP, starting_free_net.first, dummy_net,
                    dummy_net, dummy_net, starting_vertex.first->GetWidth(),
                    starting_vertex.first->GetLength()),
        Mos::Create("Dummy", Mos::Type::kN, starting_free_net.second, dummy_net,
                    dummy_net, dummy_net, starting_vertex.second->GetWidth(),
                    starting_vertex.second->GetLength())};
    path.push_back(ending_dummy);
    path.push_back(starting_dummy);
    path.insert(path.cend(), paths.at(i).cbegin(), paths.at(i).cend());
  }
  return path;
}

Edge FindFreeNetOfStartingVertex(const HamiltonPath& path) {
  // Notice that a net may connect multiple pins, it can then still be free
  // unless it is used in that many times.

  auto starting_vertex = path.front();

  auto net_count_of_p_most = std::map<std::shared_ptr<Net>, std::size_t>{};
  for (auto net : NetsOf(*starting_vertex.first)) {
    ++net_count_of_p_most[net];
  }
  auto net_count_of_n_most = std::map<std::shared_ptr<Net>, std::size_t>{};
  for (auto net : NetsOf(*starting_vertex.second)) {
    ++net_count_of_n_most[net];
  }

#ifdef DEBUG
  std::cerr << "=== Nets of " << starting_vertex.first->GetName()
            << " ===" << std::endl;
  for (const auto& [net, count] : net_count_of_p_most) {
    std::cerr << net->GetName() << " " << count << std::endl;
  }
  std::cerr << "=== Nets of " << starting_vertex.second->GetName()
            << " ===" << std::endl;
  for (const auto& [net, count] : net_count_of_n_most) {
    std::cerr << net->GetName() << " " << count << std::endl;
  }
#endif

  // Remove gate from the count.
  --net_count_of_p_most[starting_vertex.first->GetGate()];
  --net_count_of_n_most[starting_vertex.second->GetGate()];
  // Remove the connection between the starting vertex and the next vertex.
  if (path.size() != 1) {
    auto connection = FindEdgeToNeighbor(starting_vertex, path.at(1));
    --net_count_of_p_most[connection.first];
    --net_count_of_n_most[connection.second];
    assert(std::accumulate(
               net_count_of_p_most.cbegin(), net_count_of_p_most.cend(), 0,
               [](auto sum, const auto& pair) { return sum + pair.second; })
               == 1
           && "There should be only one free net.");
    assert(std::accumulate(
               net_count_of_n_most.cbegin(), net_count_of_n_most.cend(), 0,
               [](auto sum, const auto& pair) { return sum + pair.second; })
               == 1
           && "There should be only one free net.");
  }

  auto free_net = Edge{};
  for (const auto& [net, count] : net_count_of_p_most) {
    if (count == 1) {
      free_net.first = net;
      break;
    }
  }
  for (const auto& [net, count] : net_count_of_n_most) {
    if (count == 1) {
      free_net.second = net;
      break;
    }
  }

  return free_net;
}

Edge FindFreeNetOfEndingVertex(const HamiltonPath& path) {
  // Notice that a net may connect multiple pins, it can then still be free
  // unless it is used in that many times.

  auto ending_vertex = path.back();

  auto net_count_of_p_most = std::map<std::shared_ptr<Net>, std::size_t>{};
  for (auto net : NetsOf(*ending_vertex.first)) {
    ++net_count_of_p_most[net];
  }
  auto net_count_of_n_most = std::map<std::shared_ptr<Net>, std::size_t>{};
  for (auto net : NetsOf(*ending_vertex.second)) {
    ++net_count_of_n_most[net];
  }

#ifdef DEBUG
  std::cerr << "=== Nets of " << ending_vertex.first->GetName()
            << " ===" << std::endl;
  for (const auto& [net, count] : net_count_of_p_most) {
    std::cerr << net->GetName() << " " << count << std::endl;
  }
  std::cerr << "=== Nets of " << ending_vertex.second->GetName()
            << " ===" << std::endl;
  for (const auto& [net, count] : net_count_of_n_most) {
    std::cerr << net->GetName() << " " << count << std::endl;
  }
#endif

  // Remove gate from the count.
  --net_count_of_p_most[ending_vertex.first->GetGate()];
  --net_count_of_n_most[ending_vertex.second->GetGate()];
  // Remove the connection between the ending vertex and the previous vertex.
  if (path.size() != 1) {
    auto connection
        = FindEdgeToNeighbor(ending_vertex, path.at(path.size() - 2));
    --net_count_of_p_most[connection.first];
    --net_count_of_n_most[connection.second];
    assert(std::accumulate(
               net_count_of_p_most.cbegin(), net_count_of_p_most.cend(), 0,
               [](auto sum, const auto& pair) { return sum + pair.second; })
               == 1
           && "There should be only one free net.");
    assert(std::accumulate(
               net_count_of_n_most.cbegin(), net_count_of_n_most.cend(), 0,
               [](auto sum, const auto& pair) { return sum + pair.second; })
               == 1
           && "There should be only one free net.");
  }

  auto free_net = Edge{};
  for (const auto& [net, count] : net_count_of_p_most) {
    if (count == 1) {
      free_net.first = net;
      break;
    }
  }
  for (const auto& [net, count] : net_count_of_n_most) {
    if (count == 1) {
      free_net.second = net;
      break;
    }
  }

  return free_net;
}

std::vector<Edge> GetEdgesOf(const HamiltonPath& path) {
  auto edges = std::vector<Edge>{};
  edges.push_back(FindFreeNetOfStartingVertex(path));
  edges.emplace_back(path.front().first->GetGate(),
                     path.front().second->GetGate());
  for (auto i = std::size_t{1}; i < path.size(); i++) {
    edges.push_back(FindEdgeToNeighbor(path.at(i - 1), path.at(i)));
    edges.emplace_back(path.at(i).first->GetGate(),
                       path.at(i).second->GetGate());
  }
  edges.push_back(FindFreeNetOfEndingVertex(path));
  return edges;
}

std::vector<Edge> GetEdgesWithGateExcludedOf(const HamiltonPath& path) {
  auto edges = std::vector<Edge>{};
  edges.push_back(FindFreeNetOfStartingVertex(path));
  for (auto i = std::size_t{1}; i < path.size(); i++) {
    edges.push_back(FindEdgeToNeighbor(path.at(i - 1), path.at(i)));
  }
  edges.push_back(FindFreeNetOfEndingVertex(path));
  return edges;
}

std::vector<std::shared_ptr<Net>> NetsOf(const Mos& mos) {
  // NOTE: The connection of substrate doesn't count since all P MOS usually all
  // connect their substrate to the same point. So are the N MOS.
  return {mos.GetDrain(), mos.GetGate(), mos.GetSource()};
}

}  // namespace
