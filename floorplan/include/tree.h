#ifndef FLOORPLAN_TREE_H_
#define FLOORPLAN_TREE_H_

#include <cassert>
#include <iostream>
#include <random>
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

  bool IsCut() const {
    return !is_index_;
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
  enum Move {
    kOperandSwap = 1,
    kChainInvert = 2,
    kOperandAndOperatorSwap = 3,
  };

  void Perturb();

  /// @brief Restores the previous perturbation.
  /// @note Only the latest previous  perturbation can be restored.
  void Restore();

  void Dump(std::ostream& out = std::cout) const;

  SlicingTree(std::vector<Block> blocks);

 private:
  std::vector<Block> blocks_;
  std::vector<BlockIndexOrCut> prev_polish_expr_{};
  std::vector<unsigned> prev_number_of_operators_in_subexpression_{};
  std::vector<BlockIndexOrCut> polish_expr_{};
  std::vector<unsigned> number_of_operators_in_subexpression_{};

  void InitFloorplan_();

  /// @brief Stashes the current polish expression, so that it can be restored
  /// later.
  void Stash_();

  std::mt19937 twister_{std::random_device{}()};

  std::size_t SelectOperandIndex_();
  std::size_t SelectOperatorIndex_();
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_H_
