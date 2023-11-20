#ifndef FLOORPLAN_ANNEALING_H_
#define FLOORPLAN_ANNEALING_H_

#include "parser.h"
#include "tree.h"

namespace floorplan {

/// @brief Use simulate annealing to floorplan the blocks represented by the
/// tree.
/// @param tree The slicing tree representing the floorplanning of blocks.
/// @param constraint The constraint of the floorplanning.
/// @param cooling_factor How fast the temperature cools down in the annealing
/// schedule.
/// @param number_of_blocks How many blocks there are.
void SimulateAnnealing(SlicingTree& tree, Input::AspectRatio constraint,
                       double cooling_factor, unsigned number_of_blocks);

}  // namespace floorplan

#endif  // FLOORPLAN_ANNEALING_H_
