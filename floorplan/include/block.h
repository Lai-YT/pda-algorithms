#ifndef FLOORPLAN_BLOCK_H_
#define FLOORPLAN_BLOCK_H_

#include <string>

namespace floorplan {

struct Block {
  std::string name;
  unsigned width;
  unsigned height;
};
}  // namespace floorplan

#endif  // FLOORPLAN_BLOCK_H_
