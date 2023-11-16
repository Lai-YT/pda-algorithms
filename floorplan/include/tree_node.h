#ifndef FLOORPLAN_TREE_NODE_H_
#define FLOORPLAN_TREE_NODE_H_

#include <memory>

#include "cut.h"
#include "types.h"

namespace floorplan {

class TreeNode;
class BlockNode;
class CutNode;

class TreeNode {
 public:
  /// @brief The padded width of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the width of the block.
  unsigned Width() const {
    return width_;
  }
  /// @brief The padded height of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the height of the block.
  unsigned Height() const {
    return height_;
  }

  TreeNode(std::shared_ptr<TreeNode> left, std::shared_ptr<TreeNode> right)
      : left{left}, right{right} {}

  virtual ~TreeNode() = 0;

  std::weak_ptr<CutNode> parent{};
  std::shared_ptr<TreeNode> left;
  std::shared_ptr<TreeNode> right;

 protected:
  unsigned width_;
  unsigned height_;
};

class CutNode : public TreeNode {
 public:
  /// @brief Recomputes the width of the subtree.
  void UpdateWidth();

  /// @brief Recomputes the height of the subtree.
  void UpdateHeight();

  void InvertCut();

  CutNode(Cut cut, std::shared_ptr<TreeNode> left,
          std::shared_ptr<TreeNode> right)
      : TreeNode{left, right}, cut_{cut} {
    UpdateWidth();
    UpdateHeight();
  }

 private:
  Cut cut_;
};

class BlockNode : public TreeNode {
 public:
  BlockNode(ConstSharedBlockPtr block)
      : TreeNode{nullptr, nullptr}, block_{block} {
    width_ = block->width;
    height_ = block->height;
  }

 private:
  ConstSharedBlockPtr block_;
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_NODE_H_
