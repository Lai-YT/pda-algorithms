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

void BlockOrCut::SetCut(Cut cut) {
  assert(IsCut());
  block_or_cut_ = cut;
}

bool BlockOrCut::IsBlock() const {
  return std::holds_alternative<ConstSharedBlockPtr>(block_or_cut_);
}

bool BlockOrCut::IsCut() const {
  return std::holds_alternative<Cut>(block_or_cut_);
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
  // TODO: select the type of the cuts randomly
  // FIXME: the current type of initial floorplanning will never allow a swap
  // between an operand and an operator since the balloting property is held in
  // a strict manner. Swapping always violates the property.
  polish_expr_.emplace_back(BlockOrCut{blocks_.at(0)});
  number_of_operators_in_subexpression_.push_back(0);
  for (auto itr = std::next(blocks_.begin()), end = blocks_.end(); itr != end;
       ++itr) {
    polish_expr_.emplace_back(BlockOrCut{*itr});
    number_of_operators_in_subexpression_.push_back(
        number_of_operators_in_subexpression_.back());

    polish_expr_.emplace_back(BlockOrCut{Cut::kV});
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
        // invert
        polish_expr_.at(i).SetCut(
            polish_expr_.at(i).GetCut() == Cut::kH ? Cut::kV : Cut::kH);
        // Update the tree.
        assert(std::dynamic_pointer_cast<BlockNode>(polish_expr_.at(i).node)
               || std::dynamic_pointer_cast<CutNode>(polish_expr_.at(i).node));
        std::dynamic_pointer_cast<CutNode>(polish_expr_.at(i).node)
            ->InvertCut();
      }
      prev_move_ = MoveRecord_{Move::kChainInvert, {li, ui}};
    } break;
    case Move::kOperandAndOperatorSwap: {
      // Swap a pair of adjacent operand and operator
      auto opd = SelectOperandIndex_();
      // We always choose opd + 1 as the adjacent operator. If opd + 1 is not an
      // operator, select another operand.
      while (opd + 1 == polish_expr_.size()
             || !polish_expr_.at(opd + 1).IsCut()) {
        opd = SelectOperandIndex_();
      }
      auto opr = opd + 1;
      // The balloting property must hold after the move.
      // Let the number of operators in subexpression 0 ~ (opd + 1) be N_(opd +
      // 1). If 2N_(opd + 1) < opd, the balloting property holds after the move.
      // An additional data structure is used to record such N.
      // Note that since zero-indexed opd doesn't equal to the number of operand
      // + operator in the subexpression, we have to adjust it by 1.
      if (number_of_operators_in_subexpression_.at(opr) * 2 < (opd + 1)) {
        std::swap(polish_expr_.at(opd), polish_expr_.at(opr));
        number_of_operators_in_subexpression_.at(opd)
            += 1;  // the operator moves to opd
        // Update the tree.
        // Note the nodes have been swapped. opd is now the operator.
        assert(std::dynamic_pointer_cast<CutNode>(polish_expr_.at(opd).node));
        RotateLeft_(
            std::dynamic_pointer_cast<CutNode>(polish_expr_.at(opd).node));
        prev_move_ = MoveRecord_{Move::kOperandAndOperatorSwap, {opd, opr}};
      } else {
        // Don't have to restore because no move is actually made.
        Perturb();
        break;
      }
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

/// @details Swapping with the operator is equivalent to rotate the operator to
/// the left. Notice that the operand is always the right child of the operator.
/// For example, to swap b3 with H:
/// b1 b2 b3 H H b4 H -> b1 b2 H b3 H b4 H
///      H                H     //
///     / \              / \    //
///    H   b4           H  b4   //
///   / \       ->     / \      //
/// b1  [H]          [H] [b3]   //
///     / \          / \        //
///    b2 [b3]      b1  b2      //
void SlicingTree::RotateLeft_(std::shared_ptr<CutNode> opr) {
  auto opd = opr->right;
  auto parent = opr->parent.lock();
  parent->right = opd;
  opd->parent = parent;
  //         H                       //
  //        / \                      //
  // (par) H   b4         [H] (opr)  //
  //      / \             /          //
  //     b1  [b3] (opd)  b2          //

  opr->right = opr->left;
  opr->left = parent->left;
  opr->left->parent = opr;
  //      H            //
  //     / \           //
  //    H   b4   [H]   //
  //     \       / \   //
  //    [b3]   b1  b2  //

  parent->left = opr;
  // Note that the parent of opr doesn't change.

  for (auto parent = opr; parent; parent = parent->parent.lock()) {
    parent->Update();
  }
}

/// @details
/// b1 b2 b3 H H b4 H -> b1 b2 H b3 H b4 H
///        H            H        //
///       / \          / \       //
///      H  b4        H   b4     //
///     / \      ->  / \         //
///   [H] [b3]      b1  [H]      //
///   / \               / \      //
///  b1  b2            b2 [b3]   //
void SlicingTree::RotateRight_(std::shared_ptr<CutNode> opr) {
  auto parent = opr->parent.lock();
  auto opd = parent->right;
  parent->left = opr->left;
  opr->left->parent = parent;
  //         H                  //
  //        / \                 //
  // (par) H  b4      [H] (opr) //
  //      / \           \       //
  //     b1 [b3] (opr)   b2     //

  opr->left = opr->right;
  opr->right = parent->right;
  opr->right->parent = opr;
  //         H             //
  //        / \            //
  //       H  b4    [H]    //
  //      /         / \    //
  //     b1        b2 [b3] //

  parent->right = opr;
  // Note that the parent of opr doesn't change.

  for (auto parent = opr; parent; parent = parent->parent.lock()) {
    parent->Update();
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
        polish_expr_.at(i).SetCut(
            polish_expr_.at(i).GetCut() == Cut::kH ? Cut::kV : Cut::kH);
        std::dynamic_pointer_cast<CutNode>(polish_expr_.at(i).node)
            ->InvertCut();
      }
    } break;
    case Move::kOperandAndOperatorSwap: {
      auto [opr, opd] = prev_move_->index_of_nodes;
      std::swap(polish_expr_.at(opr), polish_expr_.at(opd));
      number_of_operators_in_subexpression_.at(opr)
          -= 1;  // opr is now the operand
      RotateRight_(
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
