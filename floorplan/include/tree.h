#ifndef FLOORPLAN_TREE_H_
#define FLOORPLAN_TREE_H_

#include <iostream>
#include <memory>
#include <random>
#include <variant>
#include <vector>

#include "block.h"
#include "cut.h"
#include "types.h"

namespace floorplan {

class BlockOrCut {
 public:
  ConstSharedBlockPtr GetBlock() const;

  Cut GetCut() const;

  bool IsBlock() const;

  bool IsCut() const;

  BlockOrCut(ConstSharedBlockPtr block) : block_or_cut_{block} {}
  BlockOrCut(Cut cut) : block_or_cut_{cut} {}

 private:
  std::variant<ConstSharedBlockPtr, Cut> block_or_cut_;
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

  unsigned GetArea();

  void Dump(std::ostream& out = std::cout) const;

  SlicingTree(const std::vector<Block>& blocks);

 private:
  std::vector<ConstSharedBlockPtr> blocks_;

  std::vector<BlockOrCut> polish_expr_{};
  std::vector<BlockOrCut> prev_polish_expr_{};

  /// @brief For checking the balloting property violation in O(1).
  std::vector<unsigned> number_of_operators_in_subexpression_{};
  std::vector<unsigned> prev_number_of_operators_in_subexpression_{};

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
