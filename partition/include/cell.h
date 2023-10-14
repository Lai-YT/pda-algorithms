#ifndef PARTITION_CELL_H_
#define PARTITION_CELL_H_

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
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

  BlockTag Tag() const;

  std::string_view Name() const;

  /// @brief Equivalent to the number of nets a cell connected with.
  std::size_t NumOfPins() const;

  /// @brief Connects this cell with the `net`.
  void AddNet(std::shared_ptr<Net> net);

  int gain;

  bool IsFree() const;
  void Lock();
  void Free();

  // Doubly linked list data structure used in bucket list.
  std::shared_ptr<Cell> prev{};
  std::shared_ptr<Cell> next{};

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

  /// @brief Gets the iterator which is capable of iterate over the nets.
  /// @note Iterator Pattern.
  Iterator GetNetIterator();

  Cell(std::string name) : name_{std::move(name)} {}

 private:
  std::string name_;
  /// @note The nets that contain the cell are store internal to the cell itself
  /// instead of in the CELL array.
  std::vector<std::shared_ptr<Net>> nets_{};
  BlockTag block_tag_;
  bool is_locked_{false};
};

}  // namespace partition

#endif  // PARTITION_CELL_H_
