#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "fm_partitioner.h"
#include "parser.h"

namespace partition {
class Cell;
class Net;
}  // namespace partition

using namespace partition;

int main(int argc, char const* argv[]) {
  auto cell_arr = std::vector<std::shared_ptr<partition::Cell>>{};
  auto net_arr = std::vector<std::shared_ptr<partition::Net>>{};
  auto balance_factor = 0.0;
  {  // Restrict the scope of the parser to reduce memory consumption.
    auto in = std::fstream{argv[1]};
    auto parser = Parser{in};
    parser.Parse();
    cell_arr = parser.GetCellArray();
    net_arr = parser.GetNetArray();
    balance_factor = parser.GetBalanceFactor();
  }
  auto partitioner
      = FmPartitioner{balance_factor, std::move(cell_arr), std::move(net_arr)};
  partitioner.Partition();
  std::cout << partitioner.GetCutSize() << '\n';
  return 0;
}
