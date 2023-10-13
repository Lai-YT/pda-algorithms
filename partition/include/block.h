#ifndef PARTITION_BLOCK_H_
#define PARTITION_BLOCK_H_

#include <cstddef>
#include <memory>

#include "block_tag.h"

namespace partition {

class Cell;

/// @brief A block is simply a counter.
class Block {
 public:
  std::size_t Size() const {
    return size_;
  }

  BlockTag Tag() const {
    return tag_;
  }

  /// @brief Increments the size counter.
  /// @note Duplicate cells are counted again.
  void Add(std::shared_ptr<Cell>);

  /// @brief Decrements the size counter.
  /// @note Does not check whether the cell is in the block or not.
  void Remove(std::shared_ptr<Cell>);

  Block(BlockTag tag) : tag_{tag} {}

 private:
  std::size_t size_{0};
  BlockTag tag_;
};

}  // namespace partition

#endif  // PARTITION_BLOCK_H_
