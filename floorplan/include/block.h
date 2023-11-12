#ifndef FLOORPLAN_BLOCK_H_
#define FLOORPLAN_BLOCK_H_

#include <string>

namespace floorplan {

struct Block {
  std::string name;
  unsigned width;
  unsigned height;
};

// /// @brief For blocks with up/down relationships (H cut), they have to have
// /// the same width for alignment; with left/right relationships (V cut), they
// /// have to have the same height. PaddedBlock represent a block that has its
// /// width or height padded to meet its adjacent block.
// class PaddedBlock : Block {
//   unsigned padded_width;

// };

}  // namespace floorplan

#endif  // FLOORPLAN_BLOCK_H_
