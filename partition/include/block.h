#ifndef PARTITION_BLOCK_H_
#define PARTITION_BLOCK_H_

#include <memory>
#include <unordered_set>

#include "block_tag.h"

namespace partition {

class Cell;

/// @brief A block is a set of `Cell`s connected with `Net`s.
class Block {
 public:
  std::size_t Size() const {
    // Since the size of all cells are fixed to 1, the number of cells is the
    // size of the block.
    return cells_.size();
  }

  BlockTag Tag() const {
    return tag_;
  }

  void Add(std::shared_ptr<Cell> cell);
  void Add(std::size_t cell);

  void Remove(std::shared_ptr<Cell> cell);
  void Remove(std::size_t cell);

  bool Contains(std::shared_ptr<Cell> cell) const;
  bool Contains(std::size_t cell) const;

  Block(BlockTag tag) : tag_{tag} {}

 private:
  /// @brief Offsets of the cells.
  std::unordered_set<std::size_t> cells_;
  BlockTag tag_;
};

}  // namespace partition

#endif  // PARTITION_BLOCK_H_
