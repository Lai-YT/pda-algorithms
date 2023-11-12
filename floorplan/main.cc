#include <fstream>
#include <iostream>

#include "parser.h"
#include "tree.h"

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
  std::cout << "Dump input:\n";
  std::cout << input.aspect_ratio.lower_bound << ' '
            << input.aspect_ratio.upper_bound << '\n';
  for (const auto& block : input.blocks) {
    std::cout << block.name << ' ' << block.width << ' ' << block.height
              << '\n';
  }
  auto tree = SlicingTree{input.blocks};
  std::cout << "Dump polish expression:\n";
  tree.Dump();
  for (auto i = 0; i < 10; i++) {
    std::cout << "--- " << i << " ---\n";
    tree.Perturb();
    tree.Dump();
    std::cout << "\tarea = " << tree.GetArea() << '\n';
  }
  return 0;
}
