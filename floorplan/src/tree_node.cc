#include "tree_node.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>  // operator<<

#include "block.h"
#include "cut.h"

using namespace floorplan;

TreeNode::~TreeNode() = default;

//
// CutNode
//

unsigned CutNode::Width() const {
  return composite_block_->width;
}

unsigned CutNode::Height() const {
  return composite_block_->height;
}

void CutNode::UpdateSize() {
  UpdateWidth_();
  UpdateHeight_();
}

void CutNode::UpdateWidth_() {
  if (cut_ == Cut::kH) {
    composite_block_->width = std::max(left->Width(), right->Width());
  } else {
    composite_block_->width = left->Width() + right->Width();
  }
}

void CutNode::UpdateHeight_() {
  if (cut_ == Cut::kV) {
    composite_block_->height = std::max(left->Height(), right->Height());
  } else {
    composite_block_->height = left->Height() + right->Height();
  }
}

void CutNode::InvertCut() {
  cut_ = (cut_ == Cut::kH ? Cut::kV : Cut::kH);
  UpdateSize();
}

Point CutNode::BottomLeftCoordinate() const {
  return composite_block_->bottom_left;
}

void CutNode::UpdateCoordinate(Point bottom_left) {
  // post-order traversal
  left->UpdateCoordinate(bottom_left);
  // Now we know the coordinate of the left child. It covers from bottom_left.x
  // + width over bottom_left.y + height. Base on that, we tell the right child
  // where to cover from.
  switch (cut_) {
    case Cut::kH:
      // As we're building on the top of it.
      right->UpdateCoordinate(Point{
          left->BottomLeftCoordinate().x,
          left->BottomLeftCoordinate().y + static_cast<int>(left->Height())});
      break;
    case Cut::kV:
      // As we're building on the right of it.
      right->UpdateCoordinate(Point{
          left->BottomLeftCoordinate().x + static_cast<int>(left->Width()),
          left->BottomLeftCoordinate().y});
      break;
    default:
      assert(false && "unknown kind of cut");
  }
  // The bottom left of the entire subtree is as same as its left child.
  // We are to make sure that the size are up to date.
  UpdateSize();
  composite_block_->bottom_left = left->BottomLeftCoordinate();
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

//
// BlockNode
//

unsigned BlockNode::Width() const {
  return block_->width;
}

unsigned BlockNode::Height() const {
  return block_->height;
}

Point BlockNode::BottomLeftCoordinate() const {
  return block_->bottom_left;
}

void BlockNode::UpdateCoordinate(Point bottom_left) {
  block_->bottom_left = bottom_left;
}

void BlockNode::Dump(std::ostream& out) const {
  out << block_->name << ' ';
}
