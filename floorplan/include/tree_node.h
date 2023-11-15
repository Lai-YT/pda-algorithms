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
  virtual unsigned Width() const = 0;
  /// @brief The padded height of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the height of the block.
  virtual unsigned Height() const = 0;

  TreeNode(std::shared_ptr<TreeNode> left, std::shared_ptr<TreeNode> right)
      : left{left}, right{right} {}

  std::weak_ptr<CutNode> parent{};
  std::shared_ptr<TreeNode> left;
  std::shared_ptr<TreeNode> right;
};

class CutNode : public TreeNode {
 public:
  unsigned Width() const override;

  unsigned Height() const override;

  CutNode(Cut cut, std::shared_ptr<TreeNode> left,
          std::shared_ptr<TreeNode> right)
      : TreeNode{left, right}, cut_{cut} {}

 private:
  Cut cut_;
};

class BlockNode : public TreeNode {
 public:
  unsigned Width() const override;

  unsigned Height() const override;

  BlockNode(ConstSharedBlockPtr block)
      : TreeNode{nullptr, nullptr},
        block_{block},
        width_{block->width},
        height_{block->height} {}

 private:
  ConstSharedBlockPtr block_;
  unsigned width_;
  unsigned height_;
};
}  // namespace floorplan

#endif  // FLOORPLAN_TREE_NODE_H_
