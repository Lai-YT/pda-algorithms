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
  number_of_operators_in_subexpression_.push_back(0);
  for (auto itr = std::next(blocks_.begin()), end = blocks_.end(); itr != end;
       ++itr) {
    polish_expr_.emplace_back(BlockOrCut{*itr});
    number_of_operators_in_subexpression_.push_back(
        number_of_operators_in_subexpression_.back());
    polish_expr_.emplace_back(BlockOrCut{
        std::uniform_int_distribution<>{0, 1}(twister_) == 0 ? Cut::kV
                                                             : Cut::kH});
    number_of_operators_in_subexpression_.push_back(
        number_of_operators_in_subexpression_.back() + 1);
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
  // 2. select the operand/operator to perform the move
  // 3. record this move for possible restoration
  switch (std::uniform_int_distribution<>{1, 3}(twister_)) {
    case Move::kOperandSwap: {
      // Swap 2 adjacent operands.
      // The balloting property always hold after the move. No checking is
      // required.
      auto opd = SelectOperandIndex_();
      // We always choose opd + 1 as the adjacent operand. If opd + 1 is not an
      // operand, select another opd.
      // TODO: it may be hard to find a pair of adjacent operands. Use a data
      // structure to record the pairs.
      while (opd + 1 == polish_expr_.size()
             || !polish_expr_.at(opd + 1).IsBlock()) {
        opd = SelectOperandIndex_();
      }
      std::swap(polish_expr_.at(opd), polish_expr_.at(opd + 1));
      SwapBlockNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(opd).node),
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(opd + 1).node));
      prev_move_ = MoveRecord_{Move::kOperandSwap, {opd, opd + 1}};
    } break;
    case Move::kChainInvert: {
      // Select a chain of operators to invert.
      auto op = SelectOperatorIndex_();
      // Find the lower index (li) and the upper index (ui) of the chain of
      // operators which op resides.
      // TODO: a longer chain is more likely to be choosen. Use a data structure
      // to record the chains, thus making each chain equally likely to be
      // selected.
      auto li = op;
      auto ui = op + 1;  // exclusive
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
    case Move::kOperandAndOperatorSwap: {
      // Swap a pair of adjacent operand and operator
      auto opd = SelectOperandIndex_();
      // We always choose opd - 1 as the adjacent operator. If opd - 1 is not an
      // operator, select another operand.
      // FIXME: Infinite loop occurs when all operands are on the left side
      // and operators are on the right side of the expression.
      // No operand can be selected in this configuration.
      while (opd == 0 || !polish_expr_.at(opd - 1).IsCut()) {
        opd = SelectOperandIndex_();
      }
      auto opr = opd - 1;
      // The balloting property must hold after the move.
      // Notice that we're swapping the operator to the right, which never
      // breaks the property.
      std::swap(polish_expr_.at(opd), polish_expr_.at(opr));
      number_of_operators_in_subexpression_.at(opr)
          -= 1;  // where the operator was is no longer an operator
      // Update the tree.
      // Note the nodes have been swapped. opd is now the operator.
      auto opr_node
          = std::dynamic_pointer_cast<CutNode>(polish_expr_.at(opd).node);
      assert(opr_node);
      SwapBlockNodeWithCutNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(opr).node),
          std::dynamic_pointer_cast<CutNode>(polish_expr_.at(opd).node));
      prev_move_ = MoveRecord_{Move::kOperandAndOperatorSwap, {opd, opr}};
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

void SlicingTree::SwapBlockNodeWithCutNode_(std::shared_ptr<BlockNode> opd,
                                            std::shared_ptr<CutNode> opr) {
  // There are 2 possible cases:
  // (1) The operand is the right sibling of the operator
  // For example, to swap b3 with H:
  // b1 b2 H b3 H b4 H -> b1 b2 b3 H H b4 H
  //        H              H            //
  //       / \            / \           //
  //      H  b4          H   b4         //
  //     / \       ->   / \             //
  //   [H] [b3]        b1  [H]          //
  //   / \                 / \          //
  //  b1  b2              b2 [b3]       //
  // (2) The operand is the left-most child of the right sibling of the
  // operator. For example to swap b3 with H:
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
  // operand is the same as the parent of the operator, allowing unified
  // handling.
  //
  auto parent = opr->parent.lock();
  auto parent_of_opd = opd->parent.lock();
  parent->left = opr->left;
  opr->left->parent = parent;
  //          H                     //
  //        /   \                   //
  // (par) H    b5      (opr) [H]   //
  //     /   \                  \   //
  //    b1    V  (par_opd)      b2  //
  //         /  \                   //
  // (opd) [b3]  b4                 //

  opr->left = opr->right;
  opr->right = opd;
  opd->parent = opr;
  //          H                       //
  //        /   \                     //
  // (par) H    b5      (opr) [H]     //
  //     /   \                /  \    //
  //    b1    V  (par_opd)   b2  [b3] //
  //           \                      //
  //            b4                    //

  if (parent_of_opd == parent) {
    // case (1)
    parent_of_opd->right = opr;
  } else {
    // case (2)
    parent_of_opd->left = opr;
  }
  opr->parent = parent_of_opd;

  do {  // all the way up to the root
    opr->Update();
    opr = opr->parent.lock();
  } while (opr);
}

void SlicingTree::ReverseBlockNodeWithCutNode_(std::shared_ptr<BlockNode> opd,
                                               std::shared_ptr<CutNode> opr) {
  // There are 2 possible cases:
  // (1) The operator is the right child of its parent
  // For example, to swap b3 with H:
  // b1 b2 b3 H H b4 H -> b1 b2 H b3 H b4 H
  //      H                H     //
  //     / \              / \    //
  //    H   b4           H  b4   //
  //   / \       ->     / \      //
  // b1  [H]          [H] [b3]   //
  //     / \          / \        //
  //    b2 [b3]      b1  b2      //
  // (2) The operator is the left-most inner node of a subtree. It should be
  // swapped to become the left child of the parent of the subtree to which it
  // belonged. For example, to swap b3 with H: b1 b2 b3 H b4 V H b5 H -> b1 b2
  // H b3 b4 V H b5 H
  //           H                       //
  //         /  \                      //
  //        H     b5            H      //
  //       / \                /   \    //
  //     b1   v              H    b5   //
  //         /  \    ->    /   \       //
  //       [H]  b4       [H]     V     //
  //       / \           / \    /  \   //
  //     b2 [b3]        b1  b2 [b3] b4 //
  // Notice that case (1) is a special case of case (2) where the operator is
  // the right child of its parent, , allowing unified handling.
  //
  auto parent = opr->parent.lock();
  if (parent->right == opr) {
    // case (1)
    parent->right = opd;
  } else {
    // case (2)
    parent->left = opd;
  }
  parent->left = opd;
  opd->parent = parent;
  //           H             //
  //         /  \            //
  //        H     b5         //
  //       / \               //
  //     b1   v (par)   [H]  //
  //         /  \       /    //
  //       [b3]  b4    b2    //

  opr->right = opr->left;
  auto root_of_subtree = parent;
  while (root_of_subtree != root_of_subtree->parent.lock()->right) {
    root_of_subtree = root_of_subtree->parent.lock();
  }
  auto parent_of_subtree = root_of_subtree->parent.lock();
  opr->left = parent_of_subtree->left;
  opr->left->parent = opr;
  //           H              //
  //         /  \             //
  //        H     b5          //
  //         \                //
  //          v         [H]   //
  //         /  \      /   \  //
  //       [b3]  b4   b1   b2 //

  parent_of_subtree->left = opr;
  opr->parent = parent_of_subtree;

  // parent_of_subtree is the least common ancestor, and opr is the direct
  // child of it
  opr->Update();
  for (auto ancestor_of_opd = opd->parent.lock(); ancestor_of_opd;
       ancestor_of_opd = ancestor_of_opd->parent.lock()) {
    ancestor_of_opd->Update();
  }
}

unsigned SlicingTree::GetArea() const {
  return root_->Width() * root_->Height();
}

std::size_t SlicingTree::SelectOperandIndex_() {
  auto block_or_cut
      = BlockOrCut{Cut::kH};       // a dummy initial value that not an operand
  auto expr_idx = std::size_t{0};  // a dummy initial value
  while (!block_or_cut.IsBlock()) {
    expr_idx
        = std::uniform_int_distribution<>{0, polish_expr_.size() - 1}(twister_);
    block_or_cut = polish_expr_.at(expr_idx);
  }
  return expr_idx;
}

std::size_t SlicingTree::SelectOperatorIndex_() {
  auto block_or_cut
      = BlockOrCut{0};             // a dummy initial value that not an operator
  auto expr_idx = std::size_t{0};  // a dummy initial value
  while (!block_or_cut.IsCut()) {
    expr_idx
        = std::uniform_int_distribution<>{0, polish_expr_.size() - 1}(twister_);
    block_or_cut = polish_expr_.at(expr_idx);
  }
  return expr_idx;
}

void SlicingTree::Restore() {
  assert(prev_move_ && "no previous polish expression to restore");

  // Reverses the move on the polish expression, number of operators in the
  // subexpression (if is operand/operator swap), and the tree.
  switch (prev_move_->kind_of_move) {
    case Move::kOperandSwap: {
      auto [opd_1, opd_2] = prev_move_->index_of_nodes;
      assert(opd_2 == opd_1 + 1);
      std::swap(polish_expr_.at(opd_1), polish_expr_.at(opd_2));
      SwapBlockNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(opd_1).node),
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(opd_2).node));
    } break;
    case Move::kChainInvert: {
      auto [li, ui] = prev_move_->index_of_nodes;
      for (auto i = li; i < ui; i++) {
        polish_expr_.at(i).InvertCut();
      }
    } break;
    case Move::kOperandAndOperatorSwap: {
      auto [opd, opr] = prev_move_->index_of_nodes;
      std::swap(polish_expr_.at(opd), polish_expr_.at(opr));
      number_of_operators_in_subexpression_.at(opr)
          += 1;  // where the operator was is now again the operator
      ReverseBlockNodeWithCutNode_(
          std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(opd).node),
          std::dynamic_pointer_cast<CutNode>(polish_expr_.at(opr).node));
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
