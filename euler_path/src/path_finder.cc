#include "path_finder.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "circuit.h"
#include "mos.h"

using namespace euler;

namespace {

//
// NOTE: One may say that the following functions should be member functions of
// the PathFinder, and the parameters should be the data members. However, these
// functions doesn't depend on the data members of the PathFinder, so they are
// not member functions.
//

bool IsConnected(const Mos& a, const Mos& b);
bool IsConnected(const Vertex& a, const Vertex& b);

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

/// @note In case of multiply connections, we choose one in unspecified order.
Edge FindEdgeOfTwoNeighborVertices(const Vertex& a, const Vertex& b);

/// @brief If the length of the Hamilton path is 1, nets other then the gate
// are free. Otherwise, the free net is the one that is not used as connection
// between the next vertex or the gate. Note that there may be multiple free
// nets, we choose the one that is discovered first.
Edge FindFreeNetOfStartingVertex(const HamiltonPath&);

/// @brief If the length of the Hamilton path is 1, nets other then the gate
// are free. Otherwise, the free net is the one that is not used as connection
// between the previous vertex or the gate. Note that there may be multiple free
// nets, we choose the one that is discovered first.
Edge FindFreeNetOfEndingVertex(const HamiltonPath&);

/// @return The nets that connect the MOS in the Hamilton path, including the
/// gate connections of the MOS.
std::vector<Edge> GetEdgesOf(const HamiltonPath&);

std::set<std::shared_ptr<Net>> NetsOf(const Mos&);

}  // namespace

void PathFinder::FindPath() {
  GroupVertices_();
  BuildGraph_();

  // Find a path to go through all the vertices once.
  std::cout << "=== Graph ===" << std::endl;
  for (const auto& vertex : vertices_) {
    std::cout << vertex.first->GetName() << " " << vertex.second->GetName()
              << std::endl;
    for (const auto& neighbor : adjacency_list_.at(vertex)) {
      std::cout << "  " << neighbor.first->GetName() << " "
                << neighbor.second->GetName() << std::endl;
    }
  }

  auto paths = FindHamiltonPaths_();
  std::cout << "=== Paths ===" << std::endl;
  for (const auto& path : paths) {
    for (const auto& [p, n] : path) {
      std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
    }
    std::cout << "@@@" << std::endl;
  }

  auto path = ConnectHamiltonPathOfSubgraphsWithDummy(paths);
  auto edges = GetEdgesOf(path);

  std::cout << "=== Connect Path ===" << std::endl;
  for (const auto& [p, n] : path) {
    std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
  }
  std::cout << "=== Corresponding Net ===" << std::endl;
  for (const auto& [p, n] : edges) {
    std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
  }
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
    for (const auto& p : remaining_p_mos) {
      std::cout << "Remaining P MOS: " << p->GetName() << std::endl;
    }
    for (const auto& n : remaining_n_mos) {
      std::cout << "Remaining N MOS: " << n->GetName() << std::endl;
    }

    // While sometime multiple P and N MOS share only the gate. In such case, we
    // can pair them in any way.
    assert(remaining_n_mos.size() == remaining_p_mos.size());
    for (auto i = std::size_t{0}; i < remaining_n_mos.size(); i++) {
      vertices_.emplace_back(remaining_p_mos.at(i), remaining_n_mos.at(i));
    }
  }

  std::cout << "=== MOS pairs ===" << std::endl;
  for (const auto& [p, n] : vertices_) {
    std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
  }
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
      if (IsConnected(v, v_)) {
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
  for (const auto& neighbor : adjacency_list_.at(path.back())) {
    if (to_visit.find(neighbor) != to_visit.cend()) {
      path.push_back(neighbor);
      to_visit.erase(neighbor);
      return path;
    }
  }
  for (const auto& neighbor : adjacency_list_.at(path.front())) {
    if (to_visit.find(neighbor) != to_visit.cend()) {
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
  std::cout << "=== Rotating ===" << std::endl;
  std::cout << "original: " << std::endl;
  for (const auto& [p, n] : path) {
    std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
  }
  auto rotated_paths = std::vector<HamiltonPath>{};
  // If the start or end vertex has a short cut to the vertex in the middle of
  // the path, that vertex becomes the new end or start vertex, respectively.
  // NOTE: The rotation is actually a reverse.
  for (auto i = std::size_t{2} /* skip the immediate neighbor */;
       i < path.size(); i++) {
    if (IsConnected(path.front(), path.at(i))) {
      // Make a copy for the rotating.
      auto rotated_path = path;
      std::reverse(rotated_path.begin(), rotated_path.begin() + i);
      std::cout << "left rotated: " << std::endl;
      for (const auto& [p, n] : rotated_path) {
        std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
      }
      rotated_paths.push_back(std::move(rotated_path));
    }
  }
  for (auto i = std::size_t{0};
       i < path.size() - 2 /* skip the immediate neighbor */; i++) {
    if (IsConnected(path.back(), path.at(i))) {
      auto rotated_path = path;
      // The path is broke from the left neighbor of the vertex at i.
      std::reverse(rotated_path.begin() + i + 1, rotated_path.end());
      std::cout << "right rotated: " << std::endl;
      for (const auto& [p, n] : rotated_path) {
        std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
      }
      rotated_paths.push_back(std::move(rotated_path));
    }
  }
  return rotated_paths;
}

namespace {

bool IsConnected(const Vertex& a, const Vertex& b) {
  return IsConnected(*a.first, *b.first) && IsConnected(*a.second, *b.second);
}

bool IsConnected(const Mos& a, const Mos& b) {
  return a.GetDrain() == b.GetDrain() || a.GetGate() == b.GetGate()
         || a.GetSource() == b.GetSource();
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

Edge FindEdgeOfTwoNeighborVertices(const Vertex& a, const Vertex& b) {
  auto nets_of_a = std::make_pair(NetsOf(*a.first), NetsOf(*a.second));
  auto nets_of_b = std::make_pair(NetsOf(*b.first), NetsOf(*b.second));
  std::cout << "=== P MOS ===" << std::endl;
  for (const auto& net : nets_of_a.first) {
    std::cout << "Nets of " << a.first->GetName() << ": " << net->GetName()
              << std::endl;
  }
  for (const auto& net : nets_of_b.first) {
    std::cout << "Nets of " << b.first->GetName() << ": " << net->GetName()
              << std::endl;
  }
  std::cout << "=== N MOS ===" << std::endl;
  for (const auto& net : nets_of_a.second) {
    std::cout << "Nets of " << a.second->GetName() << ": " << net->GetName()
              << std::endl;
  }
  for (const auto& net : nets_of_b.second) {
    std::cout << "Nets of " << b.second->GetName() << ": " << net->GetName()
              << std::endl;
  }
  auto p_mos_intersection = std::vector<std::shared_ptr<Net>>{};
  auto n_mos_intersection = std::vector<std::shared_ptr<Net>>{};
  std::set_intersection(nets_of_a.first.cbegin(), nets_of_a.first.cend(),
                        nets_of_b.first.cbegin(), nets_of_b.first.cend(),
                        std::back_inserter(p_mos_intersection));
  std::set_intersection(nets_of_a.second.cbegin(), nets_of_a.second.cend(),
                        nets_of_b.second.cbegin(), nets_of_b.second.cend(),
                        std::back_inserter(n_mos_intersection));
  assert(!p_mos_intersection.empty() && !n_mos_intersection.empty()
         && "The 2 vertices should have at least one common net.");
  std::cout << a.first->GetName() << " " << b.first->GetName() << std::endl;
  for (const auto& net : p_mos_intersection) {
    std::cout << "P MOS intersection: " << net->GetName() << std::endl;
  }
  std::cout << a.second->GetName() << " " << b.second->GetName() << std::endl;
  for (const auto& net : n_mos_intersection) {
    std::cout << "N MOS intersection: " << net->GetName() << std::endl;
  }
  return {p_mos_intersection.front(), n_mos_intersection.front()};
}

Edge FindFreeNetOfStartingVertex(const HamiltonPath& path) {
  auto free_net = Edge{};
  auto starting_vertex = path.begin();
  auto connection = std::make_pair<std::shared_ptr<Net>, std::shared_ptr<Net>>(
      starting_vertex->first->GetGate(), starting_vertex->second->GetGate());
  if (path.size() != 1) {
    connection = FindEdgeOfTwoNeighborVertices(*starting_vertex,
                                               *std::next(starting_vertex));
  }
  for (auto net : NetsOf(*starting_vertex->first)) {
    if (net != starting_vertex->first->GetGate() && net != connection.first) {
      free_net.first = net;
      break;
    }
  }
  for (auto net : NetsOf(*starting_vertex->second)) {
    if (net != starting_vertex->second->GetGate() && net != connection.second) {
      free_net.second = net;
      break;
    }
  }
  return free_net;
}

Edge FindFreeNetOfEndingVertex(const HamiltonPath& path) {
  auto free_net = Edge{};
  auto ending_vertex = std::prev(path.end());
  auto connection = std::make_pair<std::shared_ptr<Net>, std::shared_ptr<Net>>(
      ending_vertex->first->GetGate(), ending_vertex->second->GetGate());
  if (path.size() != 1) {
    connection = FindEdgeOfTwoNeighborVertices(*ending_vertex,
                                               *std::prev(ending_vertex));
  }
  for (auto net : NetsOf(*ending_vertex->first)) {
    if (net != ending_vertex->first->GetGate() && net != connection.first) {
      free_net.first = net;
      break;
    }
  }
  for (auto net : NetsOf(*ending_vertex->second)) {
    if (net != ending_vertex->second->GetGate() && net != connection.second) {
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
    edges.push_back(FindEdgeOfTwoNeighborVertices(path.at(i - 1), path.at(i)));
    edges.emplace_back(path.at(i).first->GetGate(),
                       path.at(i).second->GetGate());
  }
  edges.push_back(FindFreeNetOfEndingVertex(path));
  return edges;
}

std::set<std::shared_ptr<Net>> NetsOf(const Mos& mos) {
  // NOTE: The connection of substrate doesn't count since all P MOS usually all
  // connect their substrate to the same point. So are the N MOS.
  return std::set<std::shared_ptr<Net>>{mos.GetDrain(), mos.GetGate(),
                                        mos.GetSource()};
}

}  // namespace
