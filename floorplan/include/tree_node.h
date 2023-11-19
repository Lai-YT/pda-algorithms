#ifndef FLOORPLAN_TREE_NODE_H_
#define FLOORPLAN_TREE_NODE_H_

#include <iosfwd>
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

  virtual void Dump(std::ostream& out) const = 0;

  TreeNode(std::shared_ptr<TreeNode> left, std::shared_ptr<TreeNode> right)
      : left{left}, right{right} {}

  virtual ~TreeNode() = 0;

  std::weak_ptr<CutNode> parent{};
  std::shared_ptr<TreeNode> left;
  std::shared_ptr<TreeNode> right;
};

class CutNode : public TreeNode {
 public:
  /// @brief Recomputes the width and height of the subtree, ensuring
  /// synchronized updates.
  /// @note Bind the update of width and height to avoid overlooking either.
  void Update();

  void InvertCut();

  unsigned Width() const override {
    return width_;
  }

  unsigned Height() const override {
    return height_;
  }

  void Dump(std::ostream& out) const override;

  CutNode(Cut cut, std::shared_ptr<TreeNode> left,
          std::shared_ptr<TreeNode> right)
      : TreeNode{left, right}, cut_{cut} {
    Update();
  }

 private:
  Cut cut_;
  unsigned width_;
  unsigned height_;

  void UpdateWidth_();
  void UpdateHeight_();
};

class BlockNode : public TreeNode {
 public:
  unsigned Width() const override {
    return block_->width;
  }

  unsigned Height() const override {
    return block_->height;
  }

  void Dump(std::ostream& out) const override;

  BlockNode(ConstSharedBlockPtr block)
      : TreeNode{nullptr, nullptr}, block_{block} {}

 private:
  ConstSharedBlockPtr block_;
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_NODE_H_
