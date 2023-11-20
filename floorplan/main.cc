#include <cstdio>  // perror
#include <fstream>
#include <iostream>
#include <vector>

#include "annealing.h"
#include "arg.h"
#include "output_formatter.h"
#include "parser.h"
#include "tree.h"

using namespace floorplan;

int main(int argc, char* argv[]) {
  auto arg = HandleArguments(argc, argv);
  auto in = std::ifstream{arg.in};
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
    std::cout << block->name << ' ' << block->width << ' ' << block->height
              << '\n';
  }
#endif
  auto tree = SlicingTree{input.blocks};
  SimulateAnnealing(tree, input.aspect_ratio, 0.85, input.blocks.size());
  if (auto out = std::ofstream{arg.out}; arg.area_only) {
    // Outputs only the area to the file.
    out << tree.Width() * tree.Height() << '\n';
  } else {
    auto formatter = OutputFormatter{out, tree, input.blocks};
    formatter.Out();
  }
#ifdef DEBUG
  std::cout << "Dump polish expression:\n";
  tree.Dump();
#endif
  return 0;
}
