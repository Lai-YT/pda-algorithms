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

}  // namespace

void PathFinder::FindPath() const {
  auto graph = BuildGraph_();

  // Find a path to go through all the vertices once.
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

  for (const auto& path : paths) {
    for (const auto& [p, n] : path) {
      std::cout << p->GetName() << "\t" << n->GetName() << std::endl;
    }
    std::cout << "@@@" << std::endl;
  }

  // TODO: Connect the paths.
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
        if (p->GetDrain() == n->GetDrain() || p->GetSource() == n->GetSource()
            || p->GetSubstrate() == n->GetSubstrate()) {
          mos_pairs.emplace_back(p, n);
          std::cout << "[DEBUG] " << p->GetName() << " " << n->GetName()
                    << std::endl;
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

  return mos_pairs;
}

std::map<Vertex, Neighbors> PathFinder::BuildGraph_() const {
  auto graph = Graph{};
  auto mos_pairs = GroupMosPairs_();
  // Each pair is a vertex in the graph. Two vertex are neighbors if they have
  // their P MOS connected and N MOS connected.
  for (const auto& [p, n] : mos_pairs) {
    for (const auto& [p_, n_] : mos_pairs) {
      if (p == p_ || n == n_) {
        continue;
      }
      if (IsConnected(*p, *p_) && IsConnected(*n, *n_)) {
        graph[{p, n}].emplace_back(p_, n_);
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
    connected_subgraphs.push_back(connected_subgraph);
  }
  return connected_subgraphs;
}

}  // namespace
