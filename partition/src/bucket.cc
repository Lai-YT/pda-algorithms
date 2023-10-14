#include "bucket.h"

#include <algorithm>
#include <cassert>
#include <memory>

#include "cell.h"

using namespace partition;

void Bucket::Add(std::shared_ptr<Cell> cell) {
  ++size_;
  auto prev_head = list_.at(ToIndex_(cell->gain));
  if (prev_head) {
    cell->next = prev_head;
    prev_head->prev = cell;
  }
  list_.at(ToIndex_(cell->gain)) = cell;
  // Adding cells to a bucket can only affect the max gain by increment.
  max_gain_ = std::max(max_gain_, cell->gain);
}

void Bucket::Remove(std::shared_ptr<Cell> cell) {
  --size_;
  if (cell->next) {
    cell->next->prev = cell->prev;
  }
  if (cell->prev) {
    cell->prev->next = cell->next;
  } else {
    // is head
    list_.at(ToIndex_(cell->gain)) = cell->next;
  }
  cell->next = cell->prev = nullptr;
  // Update the max gain.
  // Note that we also check the original max gain itself, so if it's
  // corresponding list is not empty after the update, the max gain will not
  // be changed.
  for (;
       max_gain_ >= -static_cast<long>(pmax_) && !list_.at(ToIndex_(max_gain_));
       --max_gain_) {
    /* empty */;
  }
}

std::size_t Bucket::Pmax() const {
  return pmax_;
}

std::size_t Bucket::Size() const {
  return size_;
}

std::shared_ptr<Cell> Bucket::FirstMaxGainCell() const {
  assert(size_ != 0);
  return list_.at(ToIndex_(max_gain_));
}

std::size_t Bucket::ToIndex_(int gain) const {
  assert(gain <= static_cast<long>(pmax_));
  return static_cast<std::size_t>(gain + pmax_);
}
