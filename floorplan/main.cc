#include <fstream>
#include <iostream>

#include "parser.h"

using namespace floorplan;

int main(int argc, char const* argv[]) {
  auto in = std::fstream{argv[1]};
  if (!in) {
    std::perror(argv[1]);
    return 1;
  }
  auto parser = Parser{in};
  parser.Parse();
  auto input = parser.GetInput();
  std::cout << input.aspect_ratio.lower_bound << ' '
            << input.aspect_ratio.upper_bound << '\n';
  for (const auto& block : input.blocks) {
    std::cout << block.name << ' ' << block.width << ' ' << block.height
              << '\n';
  }
  return 0;
}
