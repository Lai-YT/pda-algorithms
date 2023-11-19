#ifndef FLOORPLAN_BLOCK_H_
#define FLOORPLAN_BLOCK_H_

#include <string>

namespace floorplan {

struct Point {
  int x;
  int y;
};

struct Block {
  std::string name;
  unsigned width;
  unsigned height;

  /// @brief The bottom-left coordinate of the block after the floorplanning.
  Point bottom_left{0, 0};
};

}  // namespace floorplan

#endif  // FLOORPLAN_BLOCK_H_
