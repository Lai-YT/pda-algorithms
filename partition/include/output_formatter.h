#ifndef PARTITION_OUTPUT_FORMATTER_H_
#define PARTITION_OUTPUT_FORMATTER_H_

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <utility>
#include <vector>

namespace partition {
class Net;
class Cell;

class OutputFormatter {
 public:
  void Out() const;

  OutputFormatter(std::ostream& out, std::vector<std::shared_ptr<Cell>> block_a,
                  std::vector<std::shared_ptr<Cell>> block_b,
                  std::size_t cut_size)
      : out_{out},
        block_a_{std::move(block_a)},
        block_b_{std::move(block_b)},
        cut_size_{cut_size} {}

 private:
  std::ostream& out_;
  std::vector<std::shared_ptr<Cell>> block_a_;
  std::vector<std::shared_ptr<Cell>> block_b_;
  std::size_t cut_size_;
};

}  // namespace partition

#endif  // PARTITION_OUTPUT_FORMATTER_H_
