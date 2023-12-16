#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "circuit.h"
#include "mos.h"
#include "path.h"
#include "path_finder.h"
#include "y.tab.hh"

using namespace euler;

extern void yylex_destroy();

auto circuit = std::shared_ptr<Circuit>{};

int main(int argc, char const* argv[]) {
  yy::parser parser{};
  int ret = parser.parse();

  yylex_destroy();

  // 0 on success, 1 otherwise
  if (ret) {
    return ret;
  }

#ifdef DEBUG
  std::cerr << "=== Circuit ===" << std::endl;
  for (const auto& mos : circuit->mos) {
    std::cerr << mos->GetName() << " " << mos->GetDrain()->GetName() << " "
              << mos->GetGate()->GetName() << " " << mos->GetSource()->GetName()
              << " " << mos->GetSubstrate()->GetName() << std::endl;
  }

  std::cerr << "=== Nets ===" << std::endl;
  for (const auto& [_, net] : circuit->nets) {
    std::cerr << net->GetName();
    for (const auto& connection : net->Connections()) {
      std::cerr << " " << connection.lock()->GetName();
    }
    std::cerr << std::endl;
  }
#endif

  auto path_finder = PathFinder{circuit};
  auto [path, edges, hpwl] = path_finder.FindPath();

  // // The first line gives the total HPWL of all nets in the SPICE netlist.
  std::cout << hpwl << std::endl;
  // The second and third lines shows the Euler path of the PMOS network in
  // terms of instance names and net names, respectively.
  auto prev_p_mos = path.head->vertex.first;
  for (auto curr = path.head; curr; curr = curr->next) {
    auto p = curr->vertex.first;
    if (p->GetName() != prev_p_mos->GetName() || p->GetName() != "Dummy") {
      std::cout << p->GetName() << " ";
    }
    prev_p_mos = p;
  }
  std::cout << std::endl;
  auto prev_p_net = edges.front().first;
  for (const auto& [p, _] : edges) {
    if (p->GetName() != prev_p_net->GetName() || p->GetName() != "Dummy") {
      std::cout << p->GetName() << " ";
    }
    prev_p_net = p;
  }
  std::cout << std::endl;
  // The fourth and fifth lines shows the Euler path of the NMOS network in
  // terms of instance names and net names, respectively.
  auto prev_n_mos = path.head->vertex.second;
  for (auto curr = path.head; curr; curr = curr->next) {
    auto n = curr->vertex.second;
    if (n->GetName() != prev_n_mos->GetName() || n->GetName() != "Dummy") {
      std::cout << n->GetName() << " ";
    }
    prev_n_mos = n;
  }
  std::cout << std::endl;
  auto prev_n_net = edges.front().second;
  for (const auto& [_, n] : edges) {
    if (n->GetName() != prev_n_net->GetName() || n->GetName() != "Dummy") {
      std::cout << n->GetName() << " ";
    }
    prev_n_net = n;
  }
  // // No end-of-file newline.

  return 0;
}
