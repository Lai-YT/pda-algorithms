#include <fstream>
#include <iostream>

#include "parser.h"

using namespace partition;

int main(int argc, char const* argv[]) {
  auto in = std::fstream{argv[1]};
  auto parser = Parser{in};
  parser.Parse();
  std::cout << parser.GetBalanceFactor() << '\n';
  std::cout << parser.GetCellArray().size() << '\n';
  std::cout << parser.GetNetArray().size() << '\n';
  return 0;
}
