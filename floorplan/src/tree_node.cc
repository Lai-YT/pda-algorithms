#include "tree_node.h"

#include <algorithm>

#include "cut.h"

using namespace floorplan;

TreeNode::~TreeNode() = default;

void CutNode::UpdateWidth() {
  if (cut_ == Cut::kH) {
    width_ = std::max(left->Width(), right->Width());
  } else {
    width_ = left->Width() + right->Width();
  }
}

void CutNode::UpdateHeight() {
  if (cut_ == Cut::kV) {
    height_ = std::max(left->Height(), right->Height());
  } else {
    height_ = left->Height() + right->Height();
  }
}

void CutNode::InvertCut() {
  cut_ = (cut_ == Cut::kH ? Cut::kV : Cut::kH);
}
