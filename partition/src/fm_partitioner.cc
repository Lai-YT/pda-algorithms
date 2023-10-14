#include "fm_partitioner.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

#include "block_tag.h"
#include "bucket.h"
#include "cell.h"
#include "net.h"

#ifndef NDEBUG
#include <iterator>
#include <numeric>
#endif
#ifdef DEBUG
#include <iostream>
#endif

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
#ifdef DEBUG
  std::cerr << "[DEBUG]"
            << " pmax = " << pmax << '\n';
#endif
  bucket_a_ = Bucket{pmax};
  bucket_b_ = Bucket{pmax};
}

void FmPartitioner::Partition() {
  InitPartition_();
#ifdef DEBUG
  auto pass_count = 1;
#endif
#ifndef NDEBUG
  auto expected_cut_size = std::size_t{0};
#endif
  while (true) {
#ifdef DEBUG
    std::cerr << "[DEBUG]"
              << " --- Pass " << pass_count++ << " ---\n";
    std::cerr << "[DEBUG]"
              << " size of block A is " << a_.Size() << '\n';
    std::cerr << "[DEBUG]"
              << " size of block B is " << b_.Size() << '\n';
#endif

    CalculateCellGains_();

#ifndef NDEBUG
    if (expected_cut_size != /* initial dummy value */ 0) {
      assert(GetCutSize() == expected_cut_size
               && "decrement of cut size doesn't match with the gain of the "
                  "pass");
    }
    expected_cut_size = GetCutSize();
#endif

    assert(bucket_a_.Size() + bucket_b_.Size() == cell_arr_.size());
    assert(bucket_a_.Size() == a_.Size());
    assert(bucket_b_.Size() == b_.Size());
    RunPass_();
    assert(history_.size() == cell_arr_.size());

    // Find the partition that can obtain the max gain and revert all moves
    // after that by flipping the block back.
    // Note that if we cannot obtain a positive gain, the max_gain_idx will
    // remain -1, thus reverts all the moves. And under this condition, the
    // partition completes.
    auto max_gain_idx = FindPartitionOfMaxPositiveBalancedGainFromHistory_();
    assert(max_gain_idx + 1 >= 0);
    RevertAllMovesAfter_(static_cast<std::size_t>(max_gain_idx + 1));
    assert(IsBalanced_(a_.Size()));
#ifndef NDEBUG
    auto max_gain = std::accumulate(
        history_.cbegin(), std::next(history_.cbegin(), max_gain_idx + 1), 0,
        [](int gain, const auto& record) { return gain + record.gain; });
    expected_cut_size -= max_gain;
#endif
    history_.clear();
    // Free all the cells.
    for (auto& cell : cell_arr_) {
      cell->Free();
    }
    if (max_gain_idx == -1) {
      break;
    }
  }
}

std::size_t FmPartitioner::GetCutSize() const {
  auto cut_size = std::count_if(net_arr_.cbegin(), net_arr_.cend(),
                                [](const auto& net) { return net->IsCut(); });
  return cut_size;
}

std::vector<std::shared_ptr<Cell>> FmPartitioner::GetBlockA() const {
  auto cells_in_block_a = std::vector<std::shared_ptr<Cell>>{};
  std::copy_if(cell_arr_.cbegin(), cell_arr_.cend(),
               std::back_inserter(cells_in_block_a), [](const auto& cell) {
                 return cell->Tag() == BlockTag::kBlockA;
               });
  assert(cells_in_block_a.size() == a_.Size());
  return cells_in_block_a;
}

std::vector<std::shared_ptr<Cell>> FmPartitioner::GetBlockB() const {
  auto cells_in_block_b = std::vector<std::shared_ptr<Cell>>{};
  std::copy_if(cell_arr_.cbegin(), cell_arr_.cend(),
               std::back_inserter(cells_in_block_b), [](const auto& cell) {
                 return cell->Tag() == BlockTag::kBlockB;
               });
  assert(cells_in_block_b.size() == b_.Size());
  return cells_in_block_b;
}

int FmPartitioner::FindPartitionOfMaxPositiveBalancedGainFromHistory_() const {
  auto curr_gain = 0;
  auto max_gain = 0;
  auto max_gain_idx = -1;
  for (std::size_t i = 0; i < history_.size(); i++) {
    curr_gain += history_.at(i).gain;
    if (curr_gain > max_gain && history_.at(i).is_balanced) {
      max_gain = curr_gain;
      max_gain_idx = i;
    }
  }
  return max_gain_idx;
}

void FmPartitioner::RevertAllMovesAfter_(std::size_t idx) {
#ifdef DEBUG
  std::cerr << "[DEBUG]"
            << " revert moves after " << idx << '\n';
#endif
  for (std::size_t i = idx; i < history_.size(); i++) {
    auto cell = history_.at(i).cell;
    if (cell->Tag() == BlockTag::kBlockA) {
      cell->MoveTo(BlockTag::kBlockB);
      a_.Remove(cell);
      b_.Add(cell);
    } else {
      cell->MoveTo(BlockTag::kBlockA);
      a_.Add(cell);
      b_.Remove(cell);
    }
  }
}

void FmPartitioner::RunPass_() {
  while (auto base_cell = ChooseBaseCell_()) {
#ifdef DEBUG
    std::cerr << "[DEBUG]"
              << " moving cell " << base_cell->Name() << "...\n";
#endif
    auto [from, to] = base_cell->Tag() == BlockTag::kBlockA ? std::tie(a_, b_)
                                                            : std::tie(b_, a_);

    // Add to the history so that we can find the maximal gain of this run.
    history_.push_back(
        Record_{base_cell->gain, base_cell, IsBalancedAfterMoving_(from, to)});
    for (auto it = base_cell->GetNetIterator(); !it.IsEnd(); it.Next()) {
      auto net = it.Get();
      auto tn = T(base_cell, net);
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
          if (neighbor->Tag() == to.Tag() && neighbor->IsFree()) {
            UpdateCellToGain_(neighbor, neighbor->gain - 1);
            // Since there's only 1 neighbor in the To block, we can break the
            // loop early.
            break;
          }
        }
      }
    }

    // change the net distribution to reflect the move
    GetBucketOf_(base_cell).Remove(base_cell);
    from.Remove(base_cell);
    to.Add(base_cell);
    base_cell->MoveTo(to.Tag());
    base_cell->Lock();

    for (auto it = base_cell->GetNetIterator(); !it.IsEnd(); it.Next()) {
      auto net = it.Get();
      // Notice that after the move, the original From block is now the To
      // block. A switch on the target of distribution. Not typo.
      auto fn = T(base_cell, net);
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
          if (neighbor->Tag() == from.Tag() && neighbor->IsFree()) {
            UpdateCellToGain_(neighbor, neighbor->gain + 1);
            // Since there's only 1 neighbor in the To block, we can break the
            // loop early.
            break;
          }
        }
      }
    }
#ifdef DEBUG
    std::cerr << "[DEBUG]"
              << " max gain of bucket A is now "
              << (bucket_a_.Size()
                      ? std::to_string(bucket_a_.FirstMaxGainCell()->gain)
                      : "\"empty\"")
              << '\n';
    std::cerr << "[DEBUG]"
              << " max gain of bucket B is now "
              << (bucket_b_.Size()
                      ? std::to_string(bucket_b_.FirstMaxGainCell()->gain)
                      : "\"empty\"")
              << '\n';
#endif
  }
}

std::shared_ptr<Cell> FmPartitioner::ChooseBaseCell_() const {
  // Consider the first cell (if any) of highest gain from each bucket array.
  auto high_a = bucket_a_.Size() ? bucket_a_.FirstMaxGainCell() : nullptr;
  auto high_b = bucket_b_.Size() ? bucket_b_.FirstMaxGainCell() : nullptr;

  // If either one is null, return the other.
  if (!high_a || !high_b) {
    return high_a ? high_a : high_b;
  }

  // Rejecting if moving would cause imbalance.
  // While the initial partition may be already imbalance, which requires
  // several moves from the bigger block to the smaller block.
  if (!IsBalancedAfterMoving_(a_, b_) && !IsBalancedAfterMoving_(b_, a_)) {
    return a_.Size() > b_.Size() ? high_a : high_b;
  }
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
#ifdef DEBUG
  std::cerr << "[DEBUG]"
            << " update gain of cell " << cell->Name() << " to " << gain
            << '\n';
#endif
  // Although this function doesn't break, some higher level logic may be wrong.
  assert(cell->gain != gain);

  auto& bucket = GetBucketOf_(cell);
  bucket.Remove(cell);
  cell->gain = gain;
  bucket.Add(cell);
}

Bucket& FmPartitioner::GetBucketOf_(std::shared_ptr<Cell> cell) {
  return cell->Tag() == BlockTag::kBlockA ? bucket_a_ : bucket_b_;
}

std::size_t FmPartitioner::F(std::shared_ptr<Cell> cell,
                             std::shared_ptr<Net> net) const {
  return cell->Tag() == BlockTag::kBlockA ? net->NumOfCellsInA()
                                          : net->NumOfCellsInB();
}

std::size_t FmPartitioner::T(std::shared_ptr<Cell> cell,
                             std::shared_ptr<Net> net) const {
  return cell->Tag() == BlockTag::kBlockB ? net->NumOfCellsInA()
                                          : net->NumOfCellsInB();
}

/// @details This functions is O(P).
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
      cell->SetBlock(BlockTag::kBlockA);
      a_.Add(cell);
    } else {
      cell->SetBlock(BlockTag::kBlockB);
      b_.Add(cell);
    }
  }
#ifdef DEBUG
  std::cerr << "[DEBUG]"
            << " initial size of block A is " << a_.Size() << '\n';
  std::cerr << "[DEBUG]"
            << " initial size of block B is " << b_.Size() << '\n';
#endif
}

/// @details This functions is O(P).
void FmPartitioner::CalculateCellGains_() {
  bucket_a_ = Bucket{bucket_a_.Pmax()};
  bucket_b_ = Bucket{bucket_b_.Pmax()};

  // Calculates the gains of each cells.
  for (auto& cell : cell_arr_) {
    cell->gain = 0;
    for (auto it = cell->GetNetIterator(); !it.IsEnd(); it.Next()) {
      auto net = it.Get();
      cell->gain += static_cast<int>(F(cell, net) == 1);
      cell->gain -= static_cast<int>(T(cell, net) == 0);
    }
#ifdef DEBUG
    std::cerr << "[DEBUG]"
              << " gain of cell " << cell->Name() << " is " << cell->gain
              << '\n';
#endif
    GetBucketOf_(cell).Add(cell);
  }
}

bool FmPartitioner::IsBalancedAfterMoving_(const Block& from,
                                           const Block& to) const {
  const auto size_of_from_after_moving = from.Size() - 1;

  return IsBalanced_(size_of_from_after_moving);
}

bool FmPartitioner::IsBalanced_(std::size_t s) const {
  // Balanced means the ratio of the block size over the number of cells is
  // between (0.5 - bf / 2, 0.5 + bf / 2).
  // NOTE: making the lb larger and the up smaller to be conservative on the
  // bounds.
  const auto lb = std::ceil((0.5 - balance_factor_ / 2) * cell_arr_.size());
  const auto ub = std::floor((0.5 + balance_factor_ / 2) * cell_arr_.size());
  return lb <= s && s <= ub;
}
