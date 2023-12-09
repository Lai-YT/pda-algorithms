#include "path_finder.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "circuit.h"
#include "mos.h"

using namespace euler;

namespace {

bool IsConnected(const Mos& a, const Mos& b) {
  return a.GetDrain() == b.GetDrain() || a.GetGate() == b.GetGate()
         || a.GetSource() == b.GetSource()
         || a.GetSubstrate() == b.GetSubstrate();
}

}  // namespace

/// @details In addressing the path finder problem, the objective is to identify
/// an Euler path for both P MOS transistors and N MOS transistors. It is
/// imperative that the paths for these two types of MOS transistors are
/// identical. To achieve this, we form pairs by grouping a P MOS transistor
/// with a corresponding N MOS transistor, based on the commonality of their
/// connections, and subsequently seek an Euler path for each pair.
void PathFinder::FindAPath() {
  BuildGraph_();

  // Find a path to go through all the vertices once.
  for (const auto& [vertex, neighbors] : graph_) {
    std::cout << vertex.first->GetName() << " " << vertex.second->GetName()
              << std::endl;
    for (const auto& neighbor : neighbors) {
      std::cout << "  " << neighbor.first->GetName() << " "
                << neighbor.second->GetName() << std::endl;
    }
  }
}

void PathFinder::GroupMosPairs_() {
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
      mos_pairs_.emplace_back(p_mos_with_same_gate.front(),
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
          mos_pairs_.emplace_back(p, n);
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
    // assert(remaining_n_mos.size() == remaining_p_mos.size());
    for (int i = 0; i < remaining_n_mos.size(); ++i) {
      mos_pairs_.emplace_back(remaining_p_mos.at(i), remaining_n_mos.at(i));
    }
  }
}

void PathFinder::BuildGraph_() {
  GroupMosPairs_();
  // Each pair is a vertex in the graph. Two vertex are neighbors if they have
  // their P MOS connected and N MOS connected.
  for (const auto& [p, n] : mos_pairs_) {
    for (const auto& [p_, n_] : mos_pairs_) {
      if (p == p_ || n == n_) {
        continue;
      }
      if (IsConnected(*p, *p_) && IsConnected(*n, *n_)) {
        graph_[{p, n}].emplace_back(p_, n_);
      }
    }
  }
}
