#include "tree_node.h"

#include <algorithm>

#include "cut.h"

using namespace floorplan;

unsigned BlockNode::Width() const {
  return width_;
}

unsigned BlockNode::Height() const {
  return height_;
}

unsigned CutNode::Width() const {
  if (cut_ == Cut::kH) {
    return std::max(left->Width(), right->Width());
  }
  return left->Width() + right->Width();
}

unsigned CutNode::Height() const {
  if (cut_ == Cut::kV) {
    return std::max(left->Height(), right->Height());
  }
  return left->Height() + right->Height();
}
