#ifndef FLOORPLAN_OUTPUT_FORMATTER_H_
#define FLOORPLAN_OUTPUT_FORMATTER_H_

#include <iosfwd>
#include <memory>
#include <vector>

#include "block.h"
#include "tree.h"

namespace floorplan {

class OutputFormatter {
 public:
  void Out() const;

  OutputFormatter(std::ostream& out, const SlicingTree& tree,
                  const std::vector<std::shared_ptr<Block>>& blocks)
      : out_{out}, tree_{tree}, blocks_{blocks} {}

 private:
  std::ostream& out_;
  const SlicingTree& tree_;
  const std::vector<std::shared_ptr<Block>>& blocks_;

  void OutBlock_(const Block&) const;
};

}  // namespace floorplan

#endif  // FLOORPLAN_OUTPUT_FORMATTER_H_
