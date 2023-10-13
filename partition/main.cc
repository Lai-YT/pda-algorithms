#include <fstream>
#include <iostream>

#include "fm_partitioner.h"
#include "parser.h"

using namespace partition;

int main(int argc, char const* argv[]) {
  auto in = std::fstream{argv[1]};
  auto parser = Parser{in};
  parser.Parse();
  auto partitioner = FmPartitioner{parser.GetBalanceFactor(),
                                   parser.GetCellArray(), parser.GetNetArray()};
  partitioner.Partition();
  std::cout << partitioner.GetCutSize() << '\n';
  return 0;
}
