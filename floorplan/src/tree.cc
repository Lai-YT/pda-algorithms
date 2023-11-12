#include "tree.h"

#include <array>
#include <cassert>
#include <memory>
#include <ostream>

using namespace floorplan;

SlicingTree::SlicingTree(std::vector<Block> blocks)
    : blocks_{std::move(blocks)} {
  assert(blocks_.size() > 1);
  InitFloorplan_();
}

void SlicingTree::InitFloorplan_() {
  const auto n = blocks_.size();
  // Initial State: we start with the Polish expression 01V2V3V... nV
  // TODO: select the type of the cuts randomly
  polish_expr_.push_back(BlockIndexOrCut{0});
  for (auto i = std::size_t{1}; i < n; i++) {
    polish_expr_.push_back(BlockIndexOrCut{i});
    polish_expr_.push_back(BlockIndexOrCut{Cut::kV});
  }
  assert(polish_expr_.size() == 2 * n - 1);
}

void SlicingTree::Dump(std::ostream& out) const {
  for (const auto& block_idx_or_cut : polish_expr_) {
    if (block_idx_or_cut.IsIndex()) {
      out << blocks_.at(block_idx_or_cut.GetIndex()).name;
    } else {
      out << (block_idx_or_cut.GetCut() == Cut::kH ? 'H' : 'V');
    }
    out << ' ';
  }
  out << '\n';
}
