#ifndef FLOORPLAN_TREE_NODE_H_
#define FLOORPLAN_TREE_NODE_H_

#include <iosfwd>
#include <memory>

#include "block.h"
#include "cut.h"

namespace floorplan {

class CutNode;

class TreeNode {
 public:
  /// @brief The padded width of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the width of the block.
  virtual unsigned Width() const = 0;
  /// @brief The padded height of the entire subtree. For blocks, which are leaf
  /// nodes, it's equal to the height of the block.
  virtual unsigned Height() const = 0;

  virtual Point BottomLeftCoordinate() const = 0;

  virtual void UpdateCoordinate(Point bottom_left) = 0;

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
  /// @note This functions must be called explicitly, i.e., an update on the
  /// child doesn't trigger the update of its parents.
  void UpdateSize();

  /// @note Only the size of this particular cut node is updated.
  void InvertCut();

  unsigned Width() const override;

  unsigned Height() const override;

  Point BottomLeftCoordinate() const override;

  void UpdateCoordinate(Point bottom_left) override;

  void Dump(std::ostream& out) const override;

  CutNode(Cut cut, std::shared_ptr<TreeNode> left,
          std::shared_ptr<TreeNode> right)
      : TreeNode{left, right}, cut_{cut} {
    UpdateSize();
  }

 private:
  Cut cut_;

  /// @brief Cut node holds a composite block, treating the entire subtree as a
  /// block.
  std::shared_ptr<Block> composite_block_ = std::make_shared<Block>();

  // For blocks with up/down relationships (H cut), they have to have the same
  // width for alignment; for those with left/right relationships (V cut), they
  // have to have the same height.

  void UpdateWidth_();
  void UpdateHeight_();
};

class BlockNode : public TreeNode {
 public:
  unsigned Width() const override;

  unsigned Height() const override;

  Point BottomLeftCoordinate() const override;

  void UpdateCoordinate(Point bottom_left) override;

  void Dump(std::ostream& out) const override;

  BlockNode(std::shared_ptr<Block> block)
      : TreeNode{nullptr, nullptr}, block_{block} {}

 private:
  std::shared_ptr<Block> block_;
};

}  // namespace floorplan

#endif  // FLOORPLAN_TREE_NODE_H_
