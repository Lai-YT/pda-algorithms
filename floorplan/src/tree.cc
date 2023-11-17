#include "tree.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <ostream>
#include <random>
#include <stack>
#include <tuple>
#include <utility>  // pair
#include <variant>

#include "cut.h"
#include "tree_node.h"

using namespace floorplan;

//
/// BlockOrCut
//

ConstSharedBlockPtr BlockOrCut::GetBlock() const {
  assert(IsBlock());
  return std::get<ConstSharedBlockPtr>(block_or_cut_);
}

Cut BlockOrCut::GetCut() const {
  assert(IsCut());
  return std::get<Cut>(block_or_cut_);
}

bool BlockOrCut::IsBlock() const {
  return std::holds_alternative<ConstSharedBlockPtr>(block_or_cut_);
}

bool BlockOrCut::IsCut() const {
  return std::holds_alternative<Cut>(block_or_cut_);
}

void BlockOrCutWithTreeNodePtr::InvertCut() {
  assert(IsCut());
  block_or_cut_ = (GetCut() == Cut::kH ? Cut::kV : Cut::kH);
  assert(std::dynamic_pointer_cast<CutNode>(node));
  std::dynamic_pointer_cast<CutNode>(node)->InvertCut();
}

//
// SlicingTree
//

SlicingTree::SlicingTree(const std::vector<Block>& blocks) {
  assert(blocks.size() > 1);
  blocks_.reserve(blocks.size());
  for (const auto& block : blocks) {
    blocks_.push_back(std::make_shared<const Block>(block));
  }
  InitFloorplanPolishExpr_();
  BuildTreeFromPolishExpr_();
}

void SlicingTree::InitFloorplanPolishExpr_() {
  // Initial State: we start with the Polish expression 01V2V3V... nV
  polish_expr_.emplace_back(BlockOrCut{blocks_.at(0)});
  for (auto itr = std::next(blocks_.begin()), end = blocks_.end(); itr != end;
       ++itr) {
    polish_expr_.emplace_back(BlockOrCut{*itr});
    polish_expr_.emplace_back(BlockOrCut{
        std::uniform_int_distribution<>{0, 1}(twister_) == 0 ? Cut::kV
                                                             : Cut::kH});
  }
  assert(polish_expr_.size() == 2 * blocks_.size() - 1);
}

void SlicingTree::BuildTreeFromPolishExpr_() {
  auto stack = std::stack<std::shared_ptr<TreeNode>>{};
  for (auto& block_or_cut : polish_expr_) {
    if (block_or_cut.IsBlock()) {
      auto leaf = std::make_shared<BlockNode>(block_or_cut.GetBlock());
      stack.push(leaf);
      // Build the query map so that we can update the tree in O(1) time.
      block_or_cut.node = leaf;
    } else {
      auto right = stack.top();
      stack.pop();
      auto left = stack.top();
      stack.pop();
      auto inode
          = std::make_shared<CutNode>(block_or_cut.GetCut(), left, right);
      block_or_cut.node = inode;

      right->parent = inode;
      left->parent = inode;
      stack.push(inode);
    }
  }
  auto root = stack.top();
  stack.pop();
  assert(stack.empty());
  root_ = root;
}

void SlicingTree::Perturb() {
  // 1. select one of the three moves
  // 2. select the block/cut to perform the move
  // 3. record this move for possible restoration
  switch (std::uniform_int_distribution<>{1, 3}(twister_)) {
    case Move::kBlockSwap: {
      // Swap 2 adjacent blocks.
      // The balloting property always hold after the move. No checking is
      // required.
      auto block = SelectIndexOfBlock_();
      // We always choose block + 1 as the adjacent block. If block + 1 is not
      // an block, select another block.
      // TODO: it may be hard to find a pair of adjacent blocks. Use a data
      // structure to record the pairs.
      while (block + 1 == polish_expr_.size()
             || !polish_expr_.at(block + 1).IsBlock()) {
        block = SelectIndexOfBlock_();
      }
      std::swap(polish_expr_.at(block), polish_expr_.at(block + 1));
      SwapBlockNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(block).node),
          std::dynamic_pointer_cast<BlockNode>(
              polish_expr_.at(block + 1).node));
      prev_move_ = MoveRecord_{Move::kBlockSwap, {block, block + 1}};
    } break;
    case Move::kChainInvert: {
      // Select a chain of cuts to invert.
      auto cut = SelectIndexOfCut_();
      // Find the lower index (li) and the upper index (ui) of the chain of
      // cuts which op resides.
      // TODO: a longer chain is more likely to be choosen. Use a data structure
      // to record the chains, thus making each chain equally likely to be
      // selected.
      auto li = cut;
      auto ui = cut + 1;  // exclusive
      while (li > 0 && polish_expr_.at(li - 1).IsCut()) {
        --li;
      }
      while (ui < polish_expr_.size() && polish_expr_.at(ui).IsCut()) {
        ++ui;
      }
      for (auto i = li; i < ui; i++) {
        polish_expr_.at(i).InvertCut();
      }
      prev_move_ = MoveRecord_{Move::kChainInvert, {li, ui}};
    } break;
    case Move::kBlockAndCutSwap: {
      // Swap a pair of adjacent block and cut
      auto block = SelectIndexOfBlock_();
      // We always choose block - 1 as the adjacent cut. If block - 1 is not a
      // cut, select another block.
      // FIXME: Infinite loop occurs when all blocks are on the left side
      // and cuts are on the right side of the expression. No block can be
      // selected in this configuration.
      while (block == 0 || !polish_expr_.at(block - 1).IsCut()) {
        block = SelectIndexOfBlock_();
      }
      auto cut = block - 1;
      // The balloting property must hold after the move.
      // Notice that we're swapping the cut to the right, which never
      // breaks the property.
      std::swap(polish_expr_.at(block), polish_expr_.at(cut));
      // Update the tree.
      // Note the nodes have been swapped. block is now the cut.
      auto cut_node
          = std::dynamic_pointer_cast<CutNode>(polish_expr_.at(block).node);
      assert(cut_node);
      SwapBlockNodeWithCutNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(cut).node),
          std::dynamic_pointer_cast<CutNode>(polish_expr_.at(block).node));
      prev_move_ = MoveRecord_{Move::kBlockAndCutSwap, {block, cut}};
    } break;
    default:
      assert(false && "unknown kind of move");
  }
}

void SlicingTree::SwapBlockNode_(std::shared_ptr<BlockNode> a,
                                 std::shared_ptr<BlockNode> b) {
  auto a_parent = a->parent.lock();
  auto b_parent = b->parent.lock();
  if (a_parent->left == a) {
    a_parent->left = b;
  } else {
    a_parent->right = b;
  }
  b->parent = a_parent;

  if (b_parent->left == b) {
    b_parent->left = a;
  } else {
    b_parent->right = a;
  }
  a->parent = b_parent;

  // TODO: not to update common ancestors twice.
  for (auto parent = a->parent.lock(); parent; parent = parent->parent.lock()) {
    parent->Update();
  }
  for (auto parent = b->parent.lock(); parent; parent = parent->parent.lock()) {
    parent->Update();
  }
}

void SlicingTree::SwapBlockNodeWithCutNode_(std::shared_ptr<BlockNode> block,
                                            std::shared_ptr<CutNode> cut) {
  // There are 2 possible cases:
  // (1) The block is the right sibling of the cut
  // For example, to swap b3 with H:
  // b1 b2 H b3 H b4 H -> b1 b2 b3 H H b4 H
  //        H              H            //
  //       / \            / \           //
  //      H  b4          H   b4         //
  //     / \       ->   / \             //
  //   [H] [b3]        b1  [H]          //
  //   / \                 / \          //
  //  b1  b2              b2 [b3]       //
  // (2) The block is the left-most child of the right sibling of the
  // cut. For example to swap b3 with H:
  // b1 b2 H b3 b4 V H b5 H -> b1 b2 b3 H b4 V H b5 H
  //                            H       //
  //                           /  \     //
  //          H               H     b5  //
  //        /   \            / \        //
  //       H    b5         b1   v       //
  //     /   \      ->         /  \     //
  //   [H]     V             [H]  b4    //
  //   / \    /  \           / \        //
  //  b1  b2 [b3] b4        b2 [b3]     //
  // Notice that case (1) is a special case of case (2) where the parent of the
  // block is the same as the parent of the cut, allowing unified
  // handling.
  //
  auto parent = cut->parent.lock();
  auto parent_of_block = block->parent.lock();
  parent->left = cut->left;
  cut->left->parent = parent;
  //          H                     //
  //        /   \                   //
  // (par) H    b5      (cut) [H]   //
  //     /   \                  \   //
  //    b1    V  (par_blk)      b2  //
  //         /  \                   //
  // (blk) [b3]  b4                 //

  cut->left = cut->right;
  cut->right = block;
  block->parent = cut;
  //          H                       //
  //        /   \                     //
  // (par) H    b5      (cut) [H]     //
  //     /   \                /  \    //
  //    b1    V  (par_blk)   b2  [b3] //
  //           \                      //
  //            b4                    //

  if (parent_of_block == parent) {
    // case (1)
    parent_of_block->right = cut;
  } else {
    // case (2)
    parent_of_block->left = cut;
  }
  cut->parent = parent_of_block;

  do {  // all the way up to the root
    cut->Update();
    cut = cut->parent.lock();
  } while (cut);
}

void SlicingTree::ReverseBlockNodeWithCutNode_(std::shared_ptr<BlockNode> block,
                                               std::shared_ptr<CutNode> cut) {
  // There are 2 possible cases:
  // (1) The cut is the right child of its parent
  // For example, to swap b3 with H:
  // b1 b2 b3 H H b4 H -> b1 b2 H b3 H b4 H
  //      H                H     //
  //     / \              / \    //
  //    H   b4           H  b4   //
  //   / \       ->     / \      //
  // b1  [H]          [H] [b3]   //
  //     / \          / \        //
  //    b2 [b3]      b1  b2      //
  // (2) The cut is the left-most inner node of a subtree. It should be
  // swapped to become the left child of the parent of the subtree to which it
  // belonged. For example, to swap b3 with H:
  // b1 b2 b3 H b4 V H b5 H -> b1 b2 H b3 b4 V H b5 H
  //           H                       //
  //         /  \                      //
  //        H     b5            H      //
  //       / \                /   \    //
  //     b1   v              H    b5   //
  //         /  \    ->    /   \       //
  //       [H]  b4       [H]     V     //
  //       / \           / \    /  \   //
  //     b2 [b3]        b1  b2 [b3] b4 //
  // Notice that case (1) is a special case of case (2) where the cut is
  // the right child of its parent, , allowing unified handling.
  //
  auto parent = cut->parent.lock();
  if (parent->right == cut) {
    // case (1)
    parent->right = block;
  } else {
    // case (2)
    parent->left = block;
  }
  parent->left = block;
  block->parent = parent;
  //               H                     //
  //              /  \                   //
  //  (par_root) H     b5                //
  //            / \                      //
  //           b1   v (par, root)   [H]  //
  //               /  \             /    //
  //             [b3]  b4          b2    //

  cut->right = cut->left;
  auto root_of_subtree = parent;
  while (root_of_subtree != root_of_subtree->parent.lock()->right) {
    root_of_subtree = root_of_subtree->parent.lock();
  }
  auto parent_of_subtree = root_of_subtree->parent.lock();
  cut->left = parent_of_subtree->left;
  cut->left->parent = cut;
  //              H               //
  //             /  \             //
  // (par_root)  H     b5          //
  //              \                //
  //   (par, root) v         [H]   //
  //              /  \      /   \  //
  //            [b3]  b4   b1   b2 //

  parent_of_subtree->left = cut;
  cut->parent = parent_of_subtree;

  // parent_of_subtree is the least common ancestor, and block is the direct
  // child of it
  cut->Update();
  for (auto ancestor_of_block = block->parent.lock(); ancestor_of_block;
       ancestor_of_block = ancestor_of_block->parent.lock()) {
    ancestor_of_block->Update();
  }
}

unsigned SlicingTree::GetArea() const {
  return root_->Width() * root_->Height();
}

std::size_t SlicingTree::SelectIndexOfBlock_() {
  auto block_or_cut
      = BlockOrCut{Cut::kH};       // a dummy initial value that's not a block
  auto expr_idx = std::size_t{0};  // a dummy initial value
  while (!block_or_cut.IsBlock()) {
    expr_idx
        = std::uniform_int_distribution<>{0, polish_expr_.size() - 1}(twister_);
    block_or_cut = polish_expr_.at(expr_idx);
  }
  return expr_idx;
}

std::size_t SlicingTree::SelectIndexOfCut_() {
  auto block_or_cut = BlockOrCut{0};  // a dummy initial value that's not a cut
  auto expr_idx = std::size_t{0};     // a dummy initial value
  while (!block_or_cut.IsCut()) {
    expr_idx
        = std::uniform_int_distribution<>{0, polish_expr_.size() - 1}(twister_);
    block_or_cut = polish_expr_.at(expr_idx);
  }
  return expr_idx;
}

void SlicingTree::Restore() {
  assert(prev_move_ && "no previous polish expression to restore");

  // Reverses the move on the polish expression and the tree.
  switch (prev_move_->kind_of_move) {
    case Move::kBlockSwap: {
      auto [block_1, block_2] = prev_move_->index_of_nodes;
      assert(block_2 == block_1 + 1);
      std::swap(polish_expr_.at(block_1), polish_expr_.at(block_2));
      SwapBlockNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(block_1).node),
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(block_2).node));
    } break;
    case Move::kChainInvert: {
      auto [li, ui] = prev_move_->index_of_nodes;
      for (auto i = li; i < ui; i++) {
        polish_expr_.at(i).InvertCut();
      }
    } break;
    case Move::kBlockAndCutSwap: {
      auto [block, cut] = prev_move_->index_of_nodes;
      std::swap(polish_expr_.at(block), polish_expr_.at(cut));
      ReverseBlockNodeWithCutNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(block).node),
          std::dynamic_pointer_cast<CutNode>(polish_expr_.at(cut).node));
    } break;
    default:
      assert(false && "unknown kind of move");
  }
  // Clears the record.
  prev_move_.reset();
}

void SlicingTree::Dump(std::ostream& out) const {
  out << "expr: ";
  for (const auto& block_or_cut : polish_expr_) {
    if (block_or_cut.IsBlock()) {
      out << block_or_cut.GetBlock()->name;
    } else {
      out << (block_or_cut.GetCut() == Cut::kH ? 'H' : 'V');
    }
    out << ' ';
  }
  out << '\n';

  out << "tree: ";
  root_->Dump(out);
  out << '\n';
}
