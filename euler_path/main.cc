#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "circuit.h"
#include "mos.h"
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

  for (const auto& mos : circuit->mos) {
    std::cout << mos->GetName() << " " << mos->GetDrain()->GetName() << " "
              << mos->GetGate()->GetName() << " " << mos->GetSource()->GetName()
              << " " << mos->GetSubstrate()->GetName() << std::endl;
  }

  for (const auto& [_, net] : circuit->nets) {
    std::cout << net->GetName();
    for (const auto& connection : net->Connections()) {
      std::cout << " " << connection.lock()->GetName();
    }
    std::cout << std::endl;
  }

  auto path_finder = PathFinder{circuit};
  path_finder.FindPath();

  return 0;
}
