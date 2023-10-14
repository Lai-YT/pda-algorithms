#ifndef PARTITION_CELL_H_
#define PARTITION_CELL_H_

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "block_tag.h"

namespace partition {

class Net;

/// @note Size of all `Cell`s are fixed to be 1.
class Cell {
 public:
  /// @brief Sets the block tag and gives the distribution to each net it's on.
  /// @note This function is intended to be called only once in the beginning.
  void SetBlock(BlockTag tag);

  /// @brief Changes the block tag to `tag` and updates the distribution of all
  /// nets it's on.
  /// @note Does nothing if moving to the block it's already in.
  void MoveTo(BlockTag tag);

  BlockTag Tag() const {
    return block_tag_;
  }

  std::string name;

  /// @brief Connects this cell with the `net`.
  void AddNet(std::shared_ptr<Net> net);

  class Iterator {
   public:
    bool IsEnd() const;
    /// @brief Advances the iterator.
    void Next();
    std::shared_ptr<Net> Get();

    /// @param cell The cell that has nets to iterate over.
    Iterator(Cell& cell) : cell_{cell} {}

   private:
    Cell& cell_;
    std::size_t i_{0};
  };

  /// @note Iterator Pattern
  Iterator GetNetIterator();

  int gain;

  bool IsFree() const;
  void Lock();
  void Free();

  // Doubly linked list data structure used in bucket list.
  std::shared_ptr<Cell> prev{};
  std::shared_ptr<Cell> next{};

  /// @brief Equivalent to the number of nets a cell connected with.
  std::size_t NumOfPins() const {
    return nets_.size();
  }

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
  BlockTag block_tag_;
  bool is_locked_{false};
};

}  // namespace partition

#endif  // PARTITION_CELL_H_
