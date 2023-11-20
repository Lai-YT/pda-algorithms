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

namespace floorplan {

class BlockOrCutWithTreeNodePtr;

class BlockOrCut {
 public:
  std::shared_ptr<Block> GetBlock() const;

  Cut GetCut() const;

  bool IsBlock() const;

  bool IsCut() const;

  explicit BlockOrCut(std::shared_ptr<Block> block) : block_or_cut_{block} {}
  explicit BlockOrCut(Cut cut) : block_or_cut_{cut} {}
  /// @note Performs a slicing copy to drop the tree node. This is done by
  /// intention.
  explicit BlockOrCut(const BlockOrCutWithTreeNodePtr&);

 protected:
  std::variant<std::shared_ptr<Block>, Cut> block_or_cut_;
};

class BlockOrCutWithTreeNodePtr : public BlockOrCut {
 public:
  using BlockOrCut::BlockOrCut;

  /// @brief Inverts the cut as well as the node.
  void InvertCut();

  BlockOrCutWithTreeNodePtr(const BlockOrCut& block_or_cut)
      : BlockOrCut{block_or_cut} {}

  std::shared_ptr<TreeNode> node{};
};

/// @brief The tree for floorplanning.
class SlicingTree {
 public:
  enum Move {
    kBlockSwap = 1,
    kChainInvert = 2,
    kBlockAndCutSwap = 3,
  };

  void Perturb();

  /// @note This function has to called explicitly to have the result of the
  /// perturbation actually affect the coordinate of the blocks.
  void UpdateCoordinateOfBlocks();

  /// @brief Restores the previous perturbation.
  /// @note Only the latest previous perturbation can be restored.
  void Restore();

  /// @brief Takes a snapshot on the polish expression.
  /// @note This is particularly for storing the minimum area between
  /// perturbations.
  std::vector<BlockOrCut> Snapshot() const;
  /// @brief Rebuilds the slicing tree from the snapshot of a polish expression.
  /// @param snapshot Must be the snapshot of this particular slicing tree.
  void RebuildFromSnapshot(const std::vector<BlockOrCut>& snapshot);

  unsigned Width() const;
  unsigned Height() const;

  void Dump(std::ostream& out = std::cout) const;

  SlicingTree(std::vector<std::shared_ptr<Block>> blocks);

 private:
  std::vector<std::shared_ptr<Block>> blocks_;

  /// @brief Record the moves so that we can restore the previous perturbation,
  /// especially to restore the tree structure. This also helps reduce memory
  /// consumption by performing a reverse move instead of copying the entire
  /// data structure.
  struct MoveRecord_ {
    Move kind_of_move;
    /// @note The index of the nodes "before" the move. For swapping between
    /// blocks and cuts, the first index is that of the block. For inverting
    /// cuts, the indices are the lower bound and upper bound (exclusive),
    /// respectively.
    std::pair<std::size_t, std::size_t> index_of_nodes;
  };
  std::optional<MoveRecord_> prev_move_{};

  /// @brief The polish expression is used for simple perturbation.
  std::vector<BlockOrCutWithTreeNodePtr> polish_expr_{};

  /// @brief A tree structure is used to update the area quickly.
  std::shared_ptr<TreeNode> root_{};

  /// @brief Indices of cuts in cut and block pairs. This information is
  /// particularly for the block/cut swap.
  /// @note Block index is implicitly cut index + 1.
  std::vector<std::size_t> cut_and_block_pair_;

  /// @brief Removes the original cut and block pair formed by the cut and adds
  /// new cut and block pairs formed by its neighbors.
  /// @param cut Index of the cut in the expression.
  /// @param index_of_pair Index of the original cut and block pair.
  /// @note This function is called after a block/cut swap. The block index is
  /// implicitly (cut index - 1).
  void UpdatePairsFormedByNeighbors_(std::size_t cut,
                                     std::size_t index_of_pair);
  /// @brief Restores cut and block pairs formed by the neighbors.
  /// @param cut Index of the cut in the expression.
  /// @note This function is called after a block/cut reversed swap. The block
  /// index is implicitly (cut index + 1).
  void RestoredPairsFormedByNeighbors_(std::size_t cut);

  void InitFloorplanPolishExpr_();
  /// @brief Builds the entire tree with respect to the polish expression and
  /// sets up the mapping.
  void BuildTreeFromPolishExpr_();

  /// @brief Updates the tree for block/block swaps.
  void SwapBlockNode_(std::shared_ptr<BlockNode>, std::shared_ptr<BlockNode>);

  /// @brief Updates the tree for block/cut swaps.
  void SwapBlockNodeWithCutNode_(std::shared_ptr<BlockNode> block,
                                 std::shared_ptr<CutNode> cut);

  /// @brief The reverse operation of the swap between block and cut.
  /// Particularly for the restoration.
  void ReverseBlockNodeWithCutNode_(std::shared_ptr<BlockNode> block,
                                    std::shared_ptr<CutNode> cut);

  std::mt19937 twister_{std::random_device{}()};

  std::size_t SelectIndexOfBlock_();
  std::size_t SelectIndexOfCut_();
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_H_
