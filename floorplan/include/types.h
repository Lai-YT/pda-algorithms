#ifndef FLOORPLAN_TYPES_H_
#define FLOORPLAN_TYPES_H_

#include <memory>

#include "block.h"

namespace floorplan {

/// @brief The block is shared as constant to avoid altering the size of the
/// it accidentally.
using ConstSharedBlockPtr = std::shared_ptr<const Block>;

}  // namespace floorplan

#endif  // FLOORPLAN_TYPES_H_
