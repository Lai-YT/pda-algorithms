#include "fm_partitioner.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <random>
#include <tuple>
#include <utility>

#ifndef NDEBUG
#include <iostream>
#endif

#include "block_tag.h"
#include "cell.h"
#include "net.h"

using namespace partition;

FmPartitioner::FmPartitioner(double balance_factor,
                             std::vector<std::shared_ptr<Cell>> cell_array,
                             std::vector<std::shared_ptr<Net>> net_array)
    : balance_factor_{balance_factor},
      cell_arr_{std::move(cell_array)},
      net_arr_{std::move(net_array)} {
  // Give size to the bucket lists.
  auto pmax = (*std::max_element(cell_arr_.cbegin(), cell_arr_.cend(),
                                 [](auto& a, auto& b) {
                                   return a->NumOfPins() < b->NumOfPins();
                                 }))
                  ->NumOfPins();
  bucket_a_.pmax = pmax;
  bucket_a_.list.resize(pmax * 2 + 1 /* -pmax ~ pmax */);
  bucket_a_.max_gain = -pmax;
  bucket_b_.pmax = pmax;
  bucket_b_.list.resize(pmax * 2 + 1 /* -pmax ~ pmax */);
  bucket_b_.max_gain = -pmax;
}

void FmPartitioner::Partition() {
  InitPartition_();
  InitDistribution_();
  InitCellGains_();

  while (auto base_cell = ChooseBaseCell_()) {
    RemoveCellFromBucket_(base_cell);

    base_cell->Lock();

    auto [from, to] = base_cell->block_tag == BlockTag::kBlockA
                          ? std::tie(a_, b_)
                          : std::tie(b_, a_);
    for (auto it = base_cell->GetNetIterator(); !it.IsEnd(); it.Next()) {
      auto net = it.Get();
      auto& fn = F(base_cell, net);
      auto& tn = T(base_cell, net);
      // check critical nets before the move
      if (tn == 0) {
        // increment gains of all free cells on the net
        for (auto it = net->GetCellIterator(); !it.IsEnd(); it.Next()) {
          auto neighbor = it.Get();
          if (neighbor->IsFree()) {
            UpdateCellToGain_(neighbor, neighbor->gain + 1);
          }
        }
      } else if (tn == 1) {
        // decrement gain of the only free cell on the net if it's free
        for (auto it = net->GetCellIterator(); !it.IsEnd(); it.Next()) {
          auto neighbor = it.Get();
          if (neighbor->block_tag == to.Tag() && neighbor->IsFree()) {
            UpdateCellToGain_(neighbor, neighbor->gain - 1);
            // Since there's only 1 neighbor in the To block, we can break the
            // loop early.
            break;
          }
        }
      }

      // change the net distribution to reflect the move
      --fn;
      from.Remove(base_cell);
      ++tn;
      to.Add(base_cell);
      base_cell->block_tag = to.Tag();

      // check critical nets after the move
      if (fn == 0) {
        // decrement gains of all free cells on the net
        for (auto it = net->GetCellIterator(); !it.IsEnd(); it.Next()) {
          auto neighbor = it.Get();
          if (neighbor->IsFree()) {
            UpdateCellToGain_(neighbor, neighbor->gain - 1);
          }
        }
      } else if (fn == 1) {
        // increment gain of the only free cell on the net if it's free
        for (auto it = net->GetCellIterator(); !it.IsEnd(); it.Next()) {
          auto neighbor = it.Get();
          if (neighbor->block_tag == to.Tag() && neighbor->IsFree()) {
            UpdateCellToGain_(neighbor, neighbor->gain + 1);
            // Since there's only 1 neighbor in the To block, we can break the
            // loop early.
            break;
          }
        }
      }
    }
  }
}

std::shared_ptr<Cell> FmPartitioner::ChooseBaseCell_() const {
  // Consider the first cell (if any) of highest gain from each bucket array.
  auto high_a = bucket_a_.size
                    ? bucket_a_.list.at(bucket_a_.ToIndex(bucket_a_.max_gain))
                    : nullptr;
  auto high_b = bucket_b_.size
                    ? bucket_b_.list.at(bucket_b_.ToIndex(bucket_b_.max_gain))
                    : nullptr;

  // If either one is null, return the other.
  if (!high_a || !high_b) {
    return high_a ? high_a : high_b;
  }

  // Rejecting if moving would cause imbalance.
  if (!IsBalancedAfterMoving_(a_, b_)) {
    return high_b;
  }
  if (!IsBalancedAfterMoving_(b_, a_)) {
    return high_a;
  }

  // If have the same gain, choose the one that gives more balance (make the
  // bigger block smaller).
  if (high_a->gain == high_b->gain) {
    return a_.Size() > b_.Size() ? high_a : high_b;
  }
  // Otherwise, choose the one with higher gain.
  return high_a->gain > high_b->gain ? high_a : high_b;
}

void FmPartitioner::UpdateCellToGain_(std::shared_ptr<Cell> cell, int gain) {
  assert(cell->gain != gain);

  RemoveCellFromBucket_(cell);

  const auto old_gain = cell->gain;
  cell->gain = gain;

  AddCellToBucket_(cell);

  auto& bucket = GetBucket_(cell);
  // Update the max gain.
  if (cell->gain > old_gain) {
    // a higher gain appears
    bucket.max_gain = cell->gain;
  } else {
    // Note that we also check the original max gain itself, so if it's
    // corresponding list is not empty after the update, the max gain will not
    // be changed.
    for (; bucket.max_gain >= -bucket.pmax
           && !bucket.list.at(bucket.ToIndex(bucket.max_gain));
         --bucket.max_gain) {
      /* empty */;
    }
  }
}

void FmPartitioner::RemoveCellFromBucket_(std::shared_ptr<Cell> cell) {
  auto& bucket = GetBucket_(cell);
  --bucket.size;
  if (cell->next) {
    cell->next->prev = cell->prev;
  }
  if (cell->prev) {
    cell->prev->next = cell->next;
  } else {
    // is head
    bucket.list.at(bucket.ToIndex(cell->gain)) = cell->next;
  }
  cell->next = cell->prev = nullptr;
}

void FmPartitioner::AddCellToBucket_(std::shared_ptr<Cell> cell) {
  auto& bucket = GetBucket_(cell);
  ++bucket.size;
  auto prev_head = bucket.list.at(bucket.ToIndex(cell->gain));
  if (prev_head) {
    cell->next = prev_head;
    prev_head->prev = cell;
  }
  bucket.list.at(bucket.ToIndex(cell->gain)) = cell;
}

FmPartitioner::Bucket_& FmPartitioner::GetBucket_(std::shared_ptr<Cell> cell) {
  return cell->block_tag == BlockTag::kBlockA ? bucket_a_ : bucket_b_;
}

std::size_t& FmPartitioner::F(std::shared_ptr<Cell> cell,
                              std::shared_ptr<Net> net) const {
  return cell->block_tag == BlockTag::kBlockA ? net->distribution.first
                                              : net->distribution.second;
}

std::size_t& FmPartitioner::T(std::shared_ptr<Cell> cell,
                              std::shared_ptr<Net> net) const {
  return cell->block_tag == BlockTag::kBlockB ? net->distribution.first
                                              : net->distribution.second;
}

void FmPartitioner::InitPartition_() {
  // TODO: One should also delete nets with only one cell and a cells that may
  // no longer be on any of the resulting nets.

  std::random_device rd{};
  auto gen = std::mt19937{rd()};
  auto dist = std::uniform_int_distribution<>{0, 1};
  for (auto& cell : cell_arr_) {
    // Each cell is equally likely to be placed in block A or block B
    // initially by flipping a coin.
    // If is head (0), put the cell in block A; if is tail (1), in block B.
    if (dist(gen) == 0) {
      cell->block_tag = BlockTag::kBlockA;
      a_.Add(cell);
    } else {
      cell->block_tag = BlockTag::kBlockB;
      b_.Add(cell);
    }
  }
#ifndef NDEBUG
  std::cerr << "[DEBUG]"
            << " initial size of block A is " << a_.Size() << '\n';
  std::cerr << "[DEBUG]"
            << " initial size of block B is " << b_.Size() << '\n';
#endif
}

/// @details This functions is O(P) if we assume the look up of the hash table
/// is constant time.
void FmPartitioner::InitDistribution_() {
  for (auto& net : net_arr_) {
    auto in_a = std::size_t{0};
    auto in_b = std::size_t{0};
    for (auto it = net->GetCellIterator(); !it.IsEnd(); it.Next()) {
      it.Get()->block_tag == BlockTag::kBlockA ? ++in_a : ++in_b;
    }
    net->distribution = {in_a, in_b};
#ifndef NDEBUG
    std::cerr << "[DEBUG]"
              << " distribution of net " << net->Offset() << " is (" << in_a
              << ", " << in_b << ")\n";
#endif
  }
}

/// @details This functions is O(P) if we assume the look up of the hash table
/// is constant time.
void FmPartitioner::InitCellGains_() {
  // Calculates the gains of each cells.
  for (auto& cell : cell_arr_) {
    cell->gain = 0;
    for (auto it = cell->GetNetIterator(); !it.IsEnd(); it.Next()) {
      auto net = it.Get();
      cell->gain += static_cast<int>(F(cell, net) == 1);
      cell->gain -= static_cast<int>(T(cell, net) == 0);
    }
#ifndef NDEBUG
    std::cerr << "[DEBUG]"
              << " gain of cell " << cell->Offset() << " is " << cell->gain
              << '\n';
#endif
    AddCellToBucket_(cell);
    auto& bucket = GetBucket_(cell);
    bucket.max_gain = std::max(bucket.max_gain, cell->gain);
  }
}

bool FmPartitioner::IsBalancedAfterMoving_(const Block& from,
                                           const Block& to) const {
  const auto num_of_cells = from.Size() + to.Size();
  const auto size_of_from_after_moving = from.Size() - 1;
  return balance_factor_ * num_of_cells - 1 <= size_of_from_after_moving
         && size_of_from_after_moving <= balance_factor_ * num_of_cells + 1;
}

std::size_t FmPartitioner::Bucket_::ToIndex(int gain) const {
  assert(gain <= pmax);
  return static_cast<std::size_t>(gain + pmax);
}
