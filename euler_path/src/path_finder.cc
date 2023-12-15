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
#include "path.h"

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

/// @brief The definition of neighbor is that the two MOS transistors have their
/// drain or source connected to another's drain or source.
bool IsNeighbor(const Vertex& a, const Vertex& b);
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
Path ConnectHamiltonPathOfSubgraphsWithDummy(const std::vector<Path>&);

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

/// @note The number of free nets in P and N MOS may not be the same.
struct FreeNets {
  std::vector<std::shared_ptr<Net>> p;
  std::vector<std::shared_ptr<Net>> n;
};

/// @return The free nets of the `fragment`, which can be used to connect to the
/// next neighbor.
FreeNets FindFreeNets(const PathFragment& fragment);

/// @return The nets that connect the MOS in the Hamilton path, including the
/// gate connections of the MOS.
std::vector<Edge> GetEdgesOf(const HamiltonPath&);

/// @note For HPWL calculation, we need to know the Hamilton distance between
/// true nets. This makes the existence of gate a noise. So we exclude the gate.
std::vector<Edge> GetEdgesWithGateExcludedOf(const HamiltonPath& path);

std::vector<std::shared_ptr<Net>> NetsOf(const Mos&);

}  // namespace

std::tuple<Path> PathFinder::FindPath() {
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
    for (auto curr = path.head; curr; curr = curr->next) {
      auto [p, n] = curr->vertex;
      std::cerr << p->GetName() << "\t" << n->GetName() << std::endl;
    }
    std::cerr << "@@@" << std::endl;
  }
#endif

  auto path = ConnectHamiltonPathOfSubgraphsWithDummy(paths);
  return path;
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

std::vector<Path> PathFinder::FindHamiltonPaths_() {
  // Select from the to visited list should be faster than iterating through all
  // the vertices and checking whether they are in the visited list.
  auto to_visit = std::set<Vertex>{vertices_.cbegin(), vertices_.cend()};
  auto paths = std::vector<Path>{};
  while (!to_visit.empty()) {
    // Randomly select a vertex to start. We select the first one for
    // simplicity.
    auto path = Path{};
    path.head = std::make_shared<PathFragment>(*to_visit.begin());
    path.tail = path.head;
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

std::optional<Path> PathFinder::Extend_(Path path,
                                        std::set<Vertex>& to_visit) const {
  // If the neighbor of the start or end vertex is not in the path, then we add
  // it into the path.
  // NOTE: If a net is already used in a connection, we cannot uses it
  // again.
  for (const auto& neighbor : adjacency_list_.at(path.tail->vertex)) {
    if (to_visit.find(neighbor) != to_visit.cend()) {
#ifdef DEBUG
      std::cerr << "Extend " << path.tail->vertex.first->GetName() << " "
                << path.tail->vertex.second->GetName() << "\tto "
                << neighbor.first->GetName() << " "
                << neighbor.second->GetName() << "...";
#endif

      auto edge = Edge{};
      auto free_nets = FindFreeNets(*path.tail);
      for (auto free_net : free_nets.p) {
        if (free_net == neighbor.first->GetSource()
            || free_net == neighbor.first->GetDrain()) {
          edge.first = free_net;
          break;
        }
      }
      for (auto free_net : free_nets.n) {
        if (free_net == neighbor.second->GetSource()
            || free_net == neighbor.second->GetDrain()) {
          edge.second = free_net;
          break;
        }
      }
      if (edge.first && edge.second) {
        path.tail->next = std::make_shared<PathFragment>(neighbor, path.tail);
        path.tail->edge_to_next = edge;
        path.tail = path.tail->next;
        to_visit.erase(neighbor);
#ifdef DEBUG
        std::cerr << "\t"
                  << "[SUCCESS]" << std::endl;
#endif
        return path;
      }
#ifdef DEBUG
      std::cerr << "\t"
                << "[FAIL]" << std::endl;
#endif
    }
  }
  for (const auto& neighbor : adjacency_list_.at(path.head->vertex)) {
    if (to_visit.find(neighbor) != to_visit.cend()) {
#ifdef DEBUG
      std::cerr << "Extend " << path.head->vertex.first->GetName() << " "
                << path.head->vertex.second->GetName() << "\tto "
                << neighbor.first->GetName() << " "
                << neighbor.second->GetName() << "...";
#endif

      auto edge = Edge{};
      auto free_nets = FindFreeNets(*path.head);
      for (auto free_net : free_nets.p) {
        if (free_net == neighbor.first->GetSource()
            || free_net == neighbor.first->GetDrain()) {
          edge.first = free_net;
          break;
        }
      }
      for (auto free_net : free_nets.n) {
        if (free_net == neighbor.second->GetSource()
            || free_net == neighbor.second->GetDrain()) {
          edge.second = free_net;
          break;
        }
      }
      if (edge.first && edge.second) {
        auto head = std::make_shared<PathFragment>(
            neighbor, std::weak_ptr<PathFragment>{}, path.head, edge);
        path.head->prev = head;
        path.head = head;
        to_visit.erase(neighbor);
#ifdef DEBUG
        std::cerr << "\t"
                  << "[SUCCESS]" << std::endl;
#endif
        return path;
      }
#ifdef DEBUG
      std::cerr << "\t"
                << "[FAIL]" << std::endl;
#endif
    }
  }
  return std::nullopt;
}

std::vector<Path> PathFinder::Rotate_(const Path& path) const {
  // Length smaller than 3 cannot be rotated.
  if (path.head == path.tail || path.head->next == path.tail) {
    return {};
  }
  auto rotated_paths = std::vector<Path>{};
  // If the start or end vertex has a short cut to the vertex in the middle of
  // the path, that vertex becomes the new end or start vertex, respectively.
  // NOTE: The rotation is actually a reverse.
  for (auto curr = path.head->next->next /* skip the immediate neighbor */;
       curr; curr = curr->next) {
    if (IsNeighbor(path.head->vertex, curr->vertex)) {
      // Make a copy for rotating.
      auto rotated_path = path;
      // Fast forward to the corresponding vertex, as we cannot manipulate the
      // original path.
      auto rotated_curr = rotated_path.head;
      while (rotated_curr->vertex != curr->vertex) {
        rotated_curr = rotated_curr->next;
      }
      // The head takes a shortcut to the curr. The link between the previous of
      // curr and the curr is broken. Then we reverse the path from the previous
      // of curr to the head.
      auto new_head = rotated_curr->prev.lock();
      new_head->next = nullptr;
      auto prev = rotated_curr;
      rotated_curr = rotated_path.head;
      while (rotated_curr) {
        auto next = rotated_curr->next;
        rotated_curr->next = prev;
        rotated_curr->next->prev = rotated_curr;
        prev = rotated_curr;
        rotated_curr = next;
      }
      rotated_paths.push_back(std::move(rotated_path));
    }
  }

  for (auto curr = path.head;
       curr->next->next /* skip the immediate neighbor */; curr = curr->next) {
    if (IsNeighbor(path.tail->vertex, curr->vertex)) {
      // Make a copy for rotating.
      auto rotated_path = path;
      // Fast forward to the corresponding vertex, as we cannot manipulate the
      // original path.
      auto rotated_curr = rotated_path.head;
      while (rotated_curr->vertex != curr->vertex) {
        rotated_curr = rotated_curr->next;
      }
      // The curr takes the shortcut to the tail. The link between
      // the curr and the next of curr is broken. Then we reverse the path from
      // the next of curr to the tail.
      rotated_curr->next->prev.reset();
      rotated_curr = rotated_curr->next;
      rotated_curr->prev.lock()->next = rotated_path.tail;
      rotated_curr->prev.reset();
      assert(!rotated_curr->prev.lock());
      rotated_path.tail = rotated_curr;
      auto prev = std::shared_ptr<PathFragment>{};
      while (rotated_curr) {
        auto next = rotated_curr->next;
        rotated_curr->next = prev;
        rotated_curr->next->prev = rotated_curr;
        prev = rotated_curr;
        rotated_curr = next;
      }
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

Path ConnectHamiltonPathOfSubgraphsWithDummy(const std::vector<Path>& paths) {
  if (paths.size() == 1) {
    return paths.front();
  }
  auto path = Path{};
  path.head = paths.front().head;
  for (auto i = std::size_t{1}; i < paths.size(); i++) {
    // Get the net that is free (not already used as an edge) to be connected
    // with the dummy.
    // The 2 dummies are connected with the dummy net.
    auto dummy_net = std::make_shared<Net>("Dummy");
    auto ending_vertex = paths.at(i - 1).tail;
    auto ending_free_net = FindFreeNets(*ending_vertex);
    assert(ending_free_net.p.size() >= 1 && ending_free_net.n.size() >= 1);
    // The size of the dummy is the same as the MOS next to it.
    auto ending_dummy
        = Vertex{Mos::Create("Dummy", Mos::Type::kP, ending_free_net.p.front(),
                             dummy_net, dummy_net, dummy_net,
                             ending_vertex->vertex.first->GetWidth(),
                             ending_vertex->vertex.first->GetLength()),
                 Mos::Create("Dummy", Mos::Type::kN, ending_free_net.n.front(),
                             dummy_net, dummy_net, dummy_net,
                             ending_vertex->vertex.second->GetWidth(),
                             ending_vertex->vertex.second->GetLength())};
    auto next_of_ending_vertex
        = std::make_shared<PathFragment>(ending_dummy, ending_vertex);
    ending_vertex->next = next_of_ending_vertex;
    ending_vertex->edge_to_next
        = {ending_free_net.p.front(), ending_free_net.n.front()};

    auto starting_vertex = paths.at(i).head;
    auto starting_free_net = FindFreeNets(*starting_vertex);
    auto starting_dummy = Vertex{
        Mos::Create("Dummy", Mos::Type::kP, starting_free_net.p.front(),
                    dummy_net, dummy_net, dummy_net,
                    starting_vertex->vertex.first->GetWidth(),
                    starting_vertex->vertex.first->GetLength()),
        Mos::Create("Dummy", Mos::Type::kN, starting_free_net.n.front(),
                    dummy_net, dummy_net, dummy_net,
                    starting_vertex->vertex.second->GetWidth(),
                    starting_vertex->vertex.second->GetLength())};
    auto prev_of_starting_vertex = std::make_shared<PathFragment>(
        starting_dummy, std::weak_ptr<PathFragment>{}, starting_vertex,
        std::make_pair(starting_free_net.p.front(),
                       starting_free_net.n.front()));
    starting_vertex->prev = prev_of_starting_vertex;

    next_of_ending_vertex->next = prev_of_starting_vertex;
    next_of_ending_vertex->edge_to_next = {dummy_net, dummy_net};
    prev_of_starting_vertex->prev = next_of_ending_vertex;
  }
  path.tail = paths.back().tail;
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
  // NOTE: The connection of the substrate doesn't count, as all P MOS typically
  // connect their substrate to the same point, and they are not used in
  // diffusion sharing. The same applies to N MOS.
  return {mos.GetDrain(), mos.GetGate(), mos.GetSource()};
}

FreeNets FindFreeNets(const PathFragment& fragment) {
#ifdef DEBUG
  std::cerr << "=== Find free nets of " << fragment.vertex.first->GetName()
            << "\t" << fragment.vertex.second->GetName() << " ===" << std::endl;
#endif

  auto net_count_of_p_most = std::map<std::shared_ptr<Net>, std::size_t>{};
  for (auto net : NetsOf(*fragment.vertex.first)) {
    ++net_count_of_p_most[net];
  }
  auto net_count_of_n_most = std::map<std::shared_ptr<Net>, std::size_t>{};
  for (auto net : NetsOf(*fragment.vertex.second)) {
    ++net_count_of_n_most[net];
  }
  // Remove gate from the count.
  --net_count_of_p_most[fragment.vertex.first->GetGate()];
  --net_count_of_n_most[fragment.vertex.second->GetGate()];
  // Remove the connection between the fragment and the next fragment, if any.
  if (fragment.next) {
    --net_count_of_p_most[fragment.edge_to_next.first];
    --net_count_of_n_most[fragment.edge_to_next.second];
  }
  // Remove the connection between the fragment and the previous fragment, if
  // any.
  if (auto prev = fragment.prev.lock()) {
    --net_count_of_p_most[prev->edge_to_next.first];
    --net_count_of_n_most[prev->edge_to_next.second];
  }
  auto free_nets = FreeNets{};
  for (const auto& [net, count] : net_count_of_p_most) {
    if (count) {
      free_nets.p.push_back(net);
    }
  }
  for (const auto& [net, count] : net_count_of_n_most) {
    if (count) {
      free_nets.n.push_back(net);
    }
  }
#ifdef DEBUG
  std::cerr << "P MOS: ";
  for (const auto& net : free_nets.p) {
    std::cerr << net->GetName() << " ";
  }
  std::cerr << std::endl;
  std::cerr << "N MOS: ";
  for (const auto& net : free_nets.n) {
    std::cerr << net->GetName() << " ";
  }
  std::cerr << std::endl;
#endif
  return free_nets;
}
}  // namespace
