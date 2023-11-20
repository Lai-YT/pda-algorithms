#include "tree_node.h"

#include <algorithm>
#include <iostream>
#include <string>  // operator<<

#include "cut.h"

using namespace floorplan;

TreeNode::~TreeNode() = default;

void CutNode::Update() {
  UpdateWidth_();
  UpdateHeight_();
}

void CutNode::UpdateWidth_() {
  if (cut_ == Cut::kH) {
    width_ = std::max(left->Width(), right->Width());
  } else {
    width_ = left->Width() + right->Width();
  }
}

void CutNode::UpdateHeight_() {
  if (cut_ == Cut::kV) {
    height_ = std::max(left->Height(), right->Height());
  } else {
    height_ = left->Height() + right->Height();
  }
}

void CutNode::InvertCut() {
  cut_ = (cut_ == Cut::kH ? Cut::kV : Cut::kH);

  Update();
  // TODO: Chained cut nodes are usually inverted together. In such cases, the
  // ancestors are updated multiple time
  for (auto parent_ = parent.lock(); parent_;
       parent_ = parent_->parent.lock()) {
    parent_->Update();
  }
}

void CutNode::Dump(std::ostream& out) const {
  // Postorder traversal.
  if (left) {
    left->Dump(out);
  }
  if (right) {
    right->Dump(out);
  }
  out << (cut_ == Cut::kH ? 'H' : 'V') << ' ';
}

void BlockNode::Dump(std::ostream& out) const {
  out << block_->name << ' ';
}
