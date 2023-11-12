#ifndef FLOORPLAN_TREE_H_
#define FLOORPLAN_TREE_H_

#include <cassert>
#include <iostream>
#include <vector>

#include "block.h"
#include "cut.h"

namespace floorplan {

class BlockIndexOrCut {
 public:
  std::size_t GetIndex() const {
    assert(is_index_);
    return index_;
  }

  Cut GetCut() const {
    assert(!is_index_);
    return cut_;
  }

  bool IsIndex() const {
    return is_index_;
  }

  BlockIndexOrCut(std::size_t index)
      : index_{index}, cut_{/* invalid */}, is_index_{true} {}
  BlockIndexOrCut(Cut cut)
      : index_{/* invalid */}, cut_{cut}, is_index_{false} {}

 private:
  std::size_t index_;
  Cut cut_;
  bool is_index_;
};

/// @brief The tree for floorplanning.
/// @note Represented using the polish expression.
class SlicingTree {
 public:
  SlicingTree(std::vector<Block> blocks);

  void Dump(std::ostream& out = std::cout) const;

 private:
  std::vector<Block> blocks_;
  std::vector<BlockIndexOrCut> polish_expr_{};

  void InitFloorplan_();
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_H_
