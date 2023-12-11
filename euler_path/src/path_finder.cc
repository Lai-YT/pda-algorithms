#include "path_finder.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
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
// functions have to be called in a specific order, by passing the result of the
// previous one as the parameter, we ensure that the order is correct. After
// that, these functions doesn't depend on the data members of the PathFinder,
// so they are not member functions.
//

bool IsConnected(const Mos& a, const Mos& b);

std::size_t DegreeOf(const Vertex&, const Graph&);

/// @brief The graph may not be fully connected. This function finds the Euler
/// path of each connected subgraphs.
std::vector<EulerPath> FindEulerPathOfSubgraphs(std::vector<std::set<Vertex>>,
                                                Graph&);

/// @param vertices The vertices of the subgraph.
/// @param graph The entire graph.
/// @note The vertices are assumed to be connected.
EulerPath FindEulerPathOfSubgraph(std::set<Vertex> vertices, Graph& graph);

/// @brief Find the connected subgraphs using DFS.
std::vector<std::set<Vertex>> FindConnectedSubgraphs(const Graph&);

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
EulerPath ConnectEulerPathOfSubgraphsWithDummy(const std::vector<EulerPath>&);

/// @note In case of multiply connections, we choose one in unspecified order.
Edge FindEdgeOfTwoNeighborVertices(const Vertex& a, const Vertex& b);

/// @brief If the length of the Euler path is 1, nets other then the gate
// are free. Otherwise, the free net is the one that is not used as connection
// between the next vertex or the gate. Note that there may be multiple free
// nets, we choose the one that is discovered first.
Edge FindFreeNetOfStartingVertex(const EulerPath&);

/// @brief If the length of the Euler path is 1, nets other then the gate
// are free. Otherwise, the free net is the one that is not used as connection
// between the previous vertex or the gate. Note that there may be multiple free
// nets, we choose the one that is discovered first.
Edge FindFreeNetOfEndingVertex(const EulerPath&);

/// @return The nets that connect the MOS in the Euler path.
std::vector<Edge> GetEdgesOf(const EulerPath&);

std::set<std::shared_ptr<Net>> NetsOf(const Mos&);

}  // namespace

void PathFinder::FindPath() const {
  auto graph = BuildGraph_();

  // Find a path to go through all the vertices once.
  std::cout << "=== Graph ===" << std::endl;
  for (const auto& [vertex, neighbors] : graph) {
    std::cout << vertex.first->GetName() << " " << vertex.second->GetName()
              << std::endl;
    for (const auto& neighbor : neighbors) {
      std::cout << "  " << neighbor.first->GetName() << " "
                << neighbor.second->GetName() << std::endl;
    }
  }

  // The graph may not be connected. We find a euler path for each connected
  // subgraph, and then connect them by adding dummy.
  auto paths = FindEulerPathOfSubgraphs(FindConnectedSubgraphs(graph), graph);
  std::cout << "=== Paths ===" << std::endl;
  for (const auto& path : paths) {
    for (const auto& [p, n] : path) {
      std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
    }
    std::cout << "@@@" << std::endl;
  }

  auto path = ConnectEulerPathOfSubgraphsWithDummy(paths);
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

std::vector<Vertex> PathFinder::GroupMosPairs_() const {
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

  auto mos_pairs = std::vector<Vertex>{};
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
      mos_pairs.emplace_back(p_mos_with_same_gate.front(),
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
          mos_pairs.emplace_back(p, n);
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
      mos_pairs.emplace_back(remaining_p_mos.at(i), remaining_n_mos.at(i));
    }
  }

  std::cout << "=== MOS pairs ===" << std::endl;
  for (const auto& [p, n] : mos_pairs) {
    std::cout << p->GetName() << " " << n->GetName() << std::endl;
  }

  return mos_pairs;
}

std::map<Vertex, Neighbors> PathFinder::BuildGraph_() const {
  auto graph = Graph{};
  auto mos_pairs = GroupMosPairs_();
  // Each pair is a vertex in the graph. Two vertex are neighbors if they have
  // their P MOS connected and N MOS connected.
  for (const auto& [p, n] : mos_pairs) {
    graph[{p, n}] = Neighbors{};
    for (const auto& [p_, n_] : mos_pairs) {
      if (p == p_ || n == n_) {
        continue;
      }
      if (IsConnected(*p, *p_) && IsConnected(*n, *n_)) {
        graph.at({p, n}).emplace_back(p_, n_);
      }
    }
  }
  return graph;
}

namespace {

bool IsConnected(const Mos& a, const Mos& b) {
  return a.GetDrain() == b.GetDrain() || a.GetGate() == b.GetGate()
         || a.GetSource() == b.GetSource()
         || a.GetSubstrate() == b.GetSubstrate();
}

std::size_t DegreeOf(const Vertex& vertex, const Graph& graph) {
  return graph.at(vertex).size();
}

std::vector<EulerPath> FindEulerPathOfSubgraphs(
    std::vector<std::set<Vertex>> vertices, Graph& graph) {
  auto paths = std::vector<EulerPath>{};
  std::cout << "Number of subgraphs: " << vertices.size() << std::endl;
  for (const auto& connected_subgraph : vertices) {
    paths.push_back(FindEulerPathOfSubgraph(connected_subgraph, graph));
  }
  return paths;
}

EulerPath FindEulerPathOfSubgraph(std::set<Vertex> vertices, Graph& graph) {
  // Hierholzer's algorithm
  auto path = EulerPath{};
  auto stack = std::vector<Vertex>{};
  auto current = *vertices.begin();
  while (!vertices.empty()) {
    if (DegreeOf(current, graph) == 0) {
      // If the current vertex has no neighbor, then it is the end of a path.
      // We add it to the path and remove it from the graph.
      path.push_back(current);
      vertices.erase(current);
      if (stack.empty()) {
        // If the stack is empty, then we have found a path.
        break;
      }
      current = stack.back();
      stack.pop_back();
    } else {
      // If the current vertex has neighbor, then we add it to the stack and go
      // to the neighbor.
      stack.push_back(current);
      // Remove such neighbor from the graph to ensure that we do not visit it
      // again.
      // NOTE: This is destructive to the graph.
      auto next_current = graph.at(current).back();
      graph.at(current).pop_back();
      current = next_current;
    }
  }
  assert(std::all_of(graph.cbegin(), graph.cend(),
                     [](const auto& vertex_neighbors) {
                       return vertex_neighbors.second.empty();
                     })
         && "The sub graph should be fully connected.");

  return path;
}

std::vector<std::set<Vertex>> FindConnectedSubgraphs(const Graph& graph) {
  auto visited = std::set<Vertex>{};
  auto connected_subgraphs = std::vector<std::set<Vertex>>{};
  for (const auto& [vertex, neighbors] : graph) {
    if (visited.find(vertex) != visited.cend()) {
      continue;
    }
    auto connected_subgraph = std::set<Vertex>{};
    auto stack = std::vector<Vertex>{vertex};
    while (!stack.empty()) {
      auto current = stack.back();
      stack.pop_back();
      if (visited.find(current) != visited.end()) {
        continue;
      }
      visited.insert(current);
      connected_subgraph.insert(current);
      for (const auto& neighbor : graph.at(current)) {
        stack.push_back(neighbor);
      }
    }
    std::cout << "Connected subgraph size: " << connected_subgraph.size()
              << std::endl;
    connected_subgraphs.push_back(connected_subgraph);
  }
  return connected_subgraphs;
}

EulerPath ConnectEulerPathOfSubgraphsWithDummy(
    const std::vector<EulerPath>& paths) {
  if (paths.size() == 1) {
    return paths.front();
  }
  auto path = EulerPath{paths.front()};
  for (auto i = std::size_t{1}; i < paths.size(); i++) {
    // Get the net that is free (not already used as an edge) to be connected
    // with the dummy.
    // The 2 dummies are connected with the dummy net.
    auto dummy_net = std::make_pair(std::make_shared<Net>("Dummy"),
                                    std::make_shared<Net>("Dummy"));
    auto ending_vertex = path.back();
    auto ending_free_net = FindFreeNetOfEndingVertex(path);
    auto gate_of_ending_dummy = std::make_shared<Net>("Dummy");
    auto ending_dummy = Vertex{
        Mos::Create("Dummy", Mos::Type::kP, ending_free_net.first,
                    gate_of_ending_dummy, dummy_net.first, dummy_net.first),
        Mos::Create("Dummy", Mos::Type::kN, ending_free_net.second,
                    gate_of_ending_dummy, dummy_net.second, dummy_net.second)};
    auto starting_vertex = paths.at(i).front();
    auto starting_free_net = FindFreeNetOfStartingVertex(paths.at(i));
    auto gate_of_starting_dummy = std::make_shared<Net>("Dummy");
    auto starting_dummy = Vertex{
        Mos::Create("Dummy", Mos::Type::kP, starting_free_net.first,
                    gate_of_starting_dummy, dummy_net.first, dummy_net.first),
        Mos::Create("Dummy", Mos::Type::kN, starting_free_net.second,
                    gate_of_starting_dummy, dummy_net.second,
                    dummy_net.second)};
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

Edge FindFreeNetOfStartingVertex(const EulerPath& path) {
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

Edge FindFreeNetOfEndingVertex(const EulerPath& path) {
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

std::vector<Edge> GetEdgesOf(const EulerPath& path) {
  auto edges = std::vector<Edge>{};
  edges.push_back(FindFreeNetOfStartingVertex(path));
  for (auto i = std::size_t{1}; i < path.size(); i++) {
    edges.push_back(FindEdgeOfTwoNeighborVertices(path.at(i - 1), path.at(i)));
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
