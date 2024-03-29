#include "annealing.h"

#include <cassert>
#include <cmath>
#include <random>
#include <vector>

#include "parser.h"
#include "tree.h"

#ifdef DEBUG
#include <iostream>
#endif

namespace {

bool IsComplyWithAspectRatioConstraint(
    unsigned width, unsigned height, floorplan::Input::AspectRatio constraint) {
  auto aspect_ratio = width / static_cast<double>(height);
  return constraint.lower_bound < aspect_ratio
         && aspect_ratio < constraint.upper_bound;
}

}  // namespace

namespace floorplan {
void SimulateAnnealing(SlicingTree& tree, Input::AspectRatio constraint,
                       double cooling_factor, unsigned number_of_blocks) {
  const auto initial_temp_unit = 100000.0;
  const auto freezing_temp = 10.0;
  const auto num_of_unit_moves_per_temp = 1u;

  auto temp = initial_temp_unit * number_of_blocks;
  const auto num_of_moves_per_temp
      = num_of_unit_moves_per_temp * number_of_blocks;

  auto twister = std::mt19937_64{std::random_device{}()};

  auto total_number_of_moves = 0u;
  // The initial floorplan may already violate the aspect ratio constraint.
  // Try as many moves as possible until the constraint is met.
  auto trials = 0u;
  while (!IsComplyWithAspectRatioConstraint(tree.Width(), tree.Height(),
                                            constraint)) {
    tree.Perturb();
    ++trials;
#ifdef DEBUG
    std::cout << "========== [TRIAL " << trials << " ] ==========\n";
    tree.Dump();
#endif
  }
  assert(IsComplyWithAspectRatioConstraint(tree.Width(), tree.Height(),
                                           constraint));
  auto min_area = tree.Width() * tree.Height();
  auto snapshot = tree.Snapshot();
  while (true) {
    auto moves = 0u;
    auto rejected_moves = 0u;
    auto uphills = 0u;
    while (moves < num_of_moves_per_temp
           && (/* downhills */ moves - uphills) < num_of_moves_per_temp / 2) {
#ifndef NDEBUG
      auto area_before_perturbation = tree.Width() * tree.Height();
#endif
      tree.Perturb();
      auto area = tree.Width() * tree.Height();
      ++moves;
      ++total_number_of_moves;
#ifdef DEBUG
      tree.Dump();
      std::cout << "\tarea = " << area << '\n';
#endif
      auto cost = static_cast<int>(area) - static_cast<int>(min_area);
#ifdef DEBUG
      std::cout << "prob = " << std::exp(-cost / temp) << '\n';
#endif
      if (IsComplyWithAspectRatioConstraint(tree.Width(), tree.Height(),
                                            constraint)
          && (cost <= 0
              || std::uniform_real_distribution<>{0, 1}(twister) < std::exp(
                     -cost / temp) /* accept uphill with probability */)) {
        if (cost > 0) {
          ++uphills;
        }
        if (area <= min_area) {
          // We accept the move on equal areas.
          min_area = area;
          snapshot = tree.Snapshot();
        }
      } else {
        tree.Restore();
        ++rejected_moves;
#ifndef NDEBUG
        auto area_after_restoration = tree.Width() * tree.Height();
        assert(area_after_restoration == area_before_perturbation);
#endif
      }
      assert(IsComplyWithAspectRatioConstraint(tree.Width(), tree.Height(),
                                               constraint));
    }
    temp *= cooling_factor;
#ifdef DEBUG
    std::cout << "rejected: "
              << rejected_moves / static_cast<double>(num_of_moves_per_temp)
              << '\n';
    std::cout << "temp: " << temp << '\n';
#endif
    if (rejected_moves / static_cast<double>(num_of_moves_per_temp) > 0.95
        || temp < freezing_temp) {
      break;
    }
  }
#ifdef DEBUG
  std::cout << "========== [SUMMARY] ==========\n";
  std::cout << trials << " trials are made\n";
  std::cout << total_number_of_moves << " moves are made\n";
#endif
  tree.RebuildFromSnapshot(snapshot);
  assert(tree.Width() * tree.Height() == min_area
         && "the tree might be broken after the rebuild");
  tree.UpdateCoordinateOfBlocks();
}

}  // namespace floorplan
