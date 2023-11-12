#include "tree.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <ostream>
#include <random>
#include <stack>

using namespace floorplan;

SlicingTree::SlicingTree(const std::vector<Block>& blocks) {
  assert(blocks.size() > 1);
  blocks_.reserve(blocks.size());
  for (const auto& block : blocks) {
    blocks_.push_back(std::make_shared<const Block>(block));
  }
  InitFloorplan_();
}

void SlicingTree::InitFloorplan_() {
  const auto n = blocks_.size();
  // Initial State: we start with the Polish expression 01V2V3V... nV
  // TODO: select the type of the cuts randomly
  polish_expr_.push_back(BlockOrCut{blocks_.at(0)});
  number_of_operators_in_subexpression_.push_back(0);
  for (auto itr = std::next(blocks_.begin()), end = blocks_.end(); itr != end;
       ++itr) {
    polish_expr_.push_back(BlockOrCut{*itr});
    number_of_operators_in_subexpression_.push_back(
        number_of_operators_in_subexpression_.back());

    polish_expr_.push_back(BlockOrCut{Cut::kV});
    number_of_operators_in_subexpression_.push_back(
        number_of_operators_in_subexpression_.back() + 1);
  }
  assert(polish_expr_.size() == 2 * n - 1);
}

void SlicingTree::Perturb() {
  Stash_();
  // 1. select one of the three moves
  // 2. select the operand/operator to perform the move
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
        polish_expr_.at(i)
            = polish_expr_.at(i).GetCut() == Cut::kH ? Cut::kV : Cut::kH;
      }
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
      // The balloting property must hold after the move.
      // Let the number of operators in subexpression 0 ~ (opd + 1) be N_(opd +
      // 1). If 2N_(opd + 1) < opd, the balloting property holds after the move.
      // An additional data structure is used to record such N.
      if (number_of_operators_in_subexpression_.at(opd + 1) * 2 < opd) {
        std::swap(polish_expr_.at(opd), polish_expr_.at(opd + 1));
        number_of_operators_in_subexpression_.at(opd)
            += 1;  // the operator moves to opd
      } else {
        Restore();
        Perturb();
        break;
      }
    } break;
    default:
      assert(false && "unknown type of move");
  }
}

unsigned SlicingTree::GetArea() {
  auto stack = std::stack<std::shared_ptr<Node>>{};
  // TODO: we first use a naive way to re-calculate the area by traversing the
  // entire tree.
  for (const auto& block_or_cut : polish_expr_) {
    if (block_or_cut.IsBlock()) {
      auto leaf = std::make_shared<BlockNode>(block_or_cut.GetBlock());
      stack.push(leaf);
    } else {
      auto right = stack.top();
      stack.pop();
      auto left = stack.top();
      stack.pop();
      auto inode
          = std::make_shared<CutNode>(block_or_cut.GetCut(), left, right);

      right->parent = inode;
      left->parent = inode;
      stack.push(inode);
    }
  }
  auto root = stack.top();
  stack.pop();
  assert(stack.empty());
  return root->Height() * root->Width();
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

void SlicingTree::Stash_() {
  prev_polish_expr_ = polish_expr_;
  prev_number_of_operators_in_subexpression_
      = number_of_operators_in_subexpression_;
}

void SlicingTree::Restore() {
  assert(!prev_polish_expr_.empty()
         && !prev_number_of_operators_in_subexpression_.empty()
         && "no previous polish expression to restore");

  polish_expr_ = std::move(prev_polish_expr_);
  prev_polish_expr_.clear();

  number_of_operators_in_subexpression_
      = std::move(prev_number_of_operators_in_subexpression_);
  prev_number_of_operators_in_subexpression_.clear();
}

void SlicingTree::Dump(std::ostream& out) const {
  for (const auto& block_or_cut : polish_expr_) {
    if (block_or_cut.IsBlock()) {
      out << block_or_cut.GetBlock()->name;
    } else {
      out << (block_or_cut.GetCut() == Cut::kH ? 'H' : 'V');
    }
    out << ' ';
  }
  out << '\n';
}
