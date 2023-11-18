#include <fstream>
#include <iostream>

#include "annealing.h"
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
#ifdef DEBUG
  std::cout << "Dump input:\n";
  std::cout << input.aspect_ratio.lower_bound << ' '
            << input.aspect_ratio.upper_bound << '\n';
  for (const auto& block : input.blocks) {
    std::cout << block.name << ' ' << block.width << ' ' << block.height
              << '\n';
  }
#endif
  auto tree = SlicingTree{input.blocks};
#ifdef DEBUG
  std::cout << "Dump polish expression:\n";
  tree.Dump();
#endif
  auto area
      = SimulateAnnealing(tree, input.aspect_ratio, 0.85, input.blocks.size());
  std::cout << area << " area\n";
  std::cout << tree.Width() / static_cast<double>(tree.Height())
            << " aspect ratio\n";
  return 0;
}
