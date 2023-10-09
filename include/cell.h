#ifndef PARTITION_CELL_H_
#define PARTITION_CELL_H_

#include <cstddef>
#include <memory>
#include <vector>

namespace partition {

class Net;

/// @note Size of all `Cell`s are fixed to be 1.
class Cell {
 public:
  /// @brief Connects this cell with the `net`.
  void AddNet(std::shared_ptr<Net> net);

  // Doubly linked list data structure used in bucket list.
  std::shared_ptr<Cell> prev{};
  std::shared_ptr<Cell> next{};

  std::size_t Offset() const {
    return offset_;
  }

  /// @param offset Where the cell locates in the cell array.
  Cell(std::size_t offset) : offset_{offset} {}

 private:
  /// @note The nets that contain the cell are store internal to the cell itself
  /// instead of in the CELL array.
  std::vector<std::shared_ptr<Net>> nets_{};
  std::size_t offset_;
};

}  // namespace partition

#endif  // PARTITION_CELL_H_
