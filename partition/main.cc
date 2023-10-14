#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "fm_partitioner.h"
#include "output_formatter.h"
#include "parser.h"

namespace partition {
class Cell;
class Net;
}  // namespace partition

using namespace partition;

void Usage(const char* prog_name);

int main(int argc, char const* argv[]) {
  if (argc < 3) {
    Usage(argv[0]);
    return 1;
  }
  //
  // Parse input.
  //
  auto cell_arr = std::vector<std::shared_ptr<partition::Cell>>{};
  auto net_arr = std::vector<std::shared_ptr<partition::Net>>{};
  auto balance_factor = 0.0;
  {  // Restrict the scope to avoid overlapping the lifetime of large data
     // structures.
    auto in = std::fstream{argv[1]};
    auto parser = Parser{in};
    parser.Parse();
    cell_arr = parser.GetCellArray();
    net_arr = parser.GetNetArray();
    balance_factor = parser.GetBalanceFactor();
  }
  //
  // Partition.
  //
  auto block_a = std::vector<std::shared_ptr<partition::Cell>>{};
  auto block_b = std::vector<std::shared_ptr<partition::Cell>>{};
  auto cut_size = 0UL;
  {  // Restrict the scope to avoid overlapping the lifetime of large data
     // structures.
    auto partitioner = FmPartitioner{balance_factor, std::move(cell_arr),
                                     std::move(net_arr)};
    partitioner.Partition();
    block_a = partitioner.GetBlockA();
    block_b = partitioner.GetBlockB();
    cut_size = partitioner.GetCutSize();
  }
  cell_arr.clear();
  net_arr.clear();
  //
  // Generate output.
  //
  {
    auto out = std::ofstream{argv[2]};
    auto formatter = OutputFormatter{out, std::move(block_a),
                                     std::move(block_b), cut_size};
    formatter.Out();
  }
  block_a.clear();
  block_b.clear();

  return 0;
}

void Usage(const char* prog_name) {
  std::cerr << "Usage: " << prog_name << " IN OUT\n";
  std::cerr << '\n';
  std::cerr << "Arguments:\n";
  std::cerr << "    IN     Name of the input net connection file\n";
  std::cerr << "    OUT    Name of the output partition result file\n";
}
