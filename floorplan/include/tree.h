#ifndef FLOORPLAN_TREE_H_
#define FLOORPLAN_TREE_H_

#include <cassert>
#include <iostream>
#include <memory>
#include <random>
#include <variant>
#include <vector>

#include "block.h"
#include "cut.h"

namespace floorplan {

/// @brief The block is shared as constant to avoid altering the size of the
/// it accidentally.
using ConstSharedBlockPtr = std::shared_ptr<const Block>;

class BlockOrCut {
 public:
  ConstSharedBlockPtr GetBlock() const {
    assert(IsBlock());
    return std::get<ConstSharedBlockPtr>(block_or_cut_);
  }

  Cut GetCut() const {
    assert(IsCut());
    return std::get<Cut>(block_or_cut_);
  }

  bool IsBlock() const {
    return std::holds_alternative<ConstSharedBlockPtr>(block_or_cut_);
  }

  bool IsCut() const {
    return std::holds_alternative<Cut>(block_or_cut_);
  }

  BlockOrCut(ConstSharedBlockPtr block) : block_or_cut_{block} {}
  BlockOrCut(Cut cut) : block_or_cut_{cut} {}

 private:
  std::variant<ConstSharedBlockPtr, Cut> block_or_cut_;
};

class Node;
class BlockNode;
class CutNode;

class Node {
 public:
  /// @brief The padded width of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the width of the block.
  virtual unsigned Width() const = 0;
  /// @brief The padded height of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the height of the block.
  virtual unsigned Height() const = 0;

  Node(std::shared_ptr<Node> left, std::shared_ptr<Node> right)
      : left{left}, right{right} {}

  std::weak_ptr<CutNode> parent{};
  std::shared_ptr<Node> left;
  std::shared_ptr<Node> right;
};

class CutNode : public Node {
 public:
  unsigned Width() const override {
    if (cut_ == Cut::kH) {
      return std::max(left->Width(), right->Width());
    }
    return left->Width() + right->Width();
  }

  unsigned Height() const override {
    if (cut_ == Cut::kV) {
      return std::max(left->Height(), right->Height());
    }
    return left->Height() + right->Height();
  }

  CutNode(Cut cut, std::shared_ptr<Node> left, std::shared_ptr<Node> right)
      : Node{left, right}, cut_{cut} {}

 private:
  Cut cut_;
};

class BlockNode : public Node {
 public:
  unsigned Width() const override {
    return width_;
  }

  unsigned Height() const override {
    return height_;
  }

  BlockNode(ConstSharedBlockPtr block)
      : Node{nullptr, nullptr},
        block_{block},
        width_{block->width},
        height_{block->height} {}

 private:
  ConstSharedBlockPtr block_;
  unsigned width_;
  unsigned height_;
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
