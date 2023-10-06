#ifndef PARTITION_CELL_H_
#define PARTITION_CELL_H_

#include <memory>
#include <vector>

namespace partition {

class Net;

class Cell {
 public:
  /// @brief Connects this cell with the `net`.
  void AddNet(std::shared_ptr<Net> net);

  // Doubly linked list data structure used in bucket list.
  std::shared_ptr<Cell> prev{};
  std::shared_ptr<Cell> next{};

 private:
  std::vector<std::shared_ptr<Net>> nets_{};
};

}  // namespace partition

#endif  // PARTITION_CELL_H_
