#ifndef FLOORPLAN_TREE_H_
#define FLOORPLAN_TREE_H_

#include <iostream>
#include <memory>  // shared_ptr
#include <optional>
#include <random>   // mt19937, random_device
#include <utility>  // pair
#include <variant>
#include <vector>

#include "block.h"
#include "cut.h"
#include "tree_node.h"
#include "types.h"

namespace floorplan {

class BlockOrCut {
 public:
  ConstSharedBlockPtr GetBlock() const;

  // FIXME: broken encapsulation

  Cut GetCut() const;
  void SetCut(Cut);

  bool IsBlock() const;

  bool IsCut() const;

  explicit BlockOrCut(ConstSharedBlockPtr block) : block_or_cut_{block} {}
  explicit BlockOrCut(Cut cut) : block_or_cut_{cut} {}

 private:
  std::variant<ConstSharedBlockPtr, Cut> block_or_cut_;
};

class BlockOrCutWithTreeNodePtr : public BlockOrCut {
 public:
  using BlockOrCut::BlockOrCut;

  BlockOrCutWithTreeNodePtr(const BlockOrCut& block_or_cut)
      : BlockOrCut{block_or_cut} {}

  std::shared_ptr<TreeNode> node{};
};

/// @brief The tree for floorplanning.
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

  unsigned GetArea() const;

  void Dump(std::ostream& out = std::cout) const;

  SlicingTree(const std::vector<Block>& blocks);

 private:
  std::vector<ConstSharedBlockPtr> blocks_;

  /// @brief Record the moves so that we can restore the previous perturbation,
  /// especially to restore the tree structure. This also helps reduce memory
  /// consumption by performing a reverse move instead of copying the entire
  /// data structure.
  struct MoveRecord_ {
    Move kind_of_move;
    /// @note The index of the nodes "before" the move. For swapping between
    /// operands and operators, the first index is that of the operand. For
    /// inverting operators, the indices are the lower bound and upper bound
    /// (exclusive), respectively.
    std::pair<std::size_t, std::size_t> index_of_nodes;
  };
  std::optional<MoveRecord_> prev_move_{};

  /// @brief The polish expression is used for simple perturbation.
  std::vector<BlockOrCutWithTreeNodePtr> polish_expr_{};

  /// @brief A tree structure is used to update the area quickly.
  std::shared_ptr<TreeNode> root_{};

  /// @brief For checking the balloting property violation in O(1).
  std::vector<unsigned> number_of_operators_in_subexpression_{};

  void InitFloorplanPolishExpr_();
  /// @brief Builds the entire tree with respect to the polish expression and
  /// sets up the mapping.
  void BuildTreeFromPolishExpr_();

  /// @brief Updates the tree for operand/operand swaps.
  void SwapBlockNode_(std::shared_ptr<BlockNode>, std::shared_ptr<BlockNode>);

  /// @brief Updates the tree for operand/operator swaps.
  /// @param opr The operator (cut node) to swap with.
  void RotateLeft_(std::shared_ptr<CutNode> opr);

  /// @brief The reverse operation of rotate left. Particularly for the
  /// restoration.
  void RotateRight_(std::shared_ptr<CutNode> opr);

  std::mt19937 twister_{std::random_device{}()};

  std::size_t SelectOperandIndex_();
  std::size_t SelectOperatorIndex_();
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_H_
