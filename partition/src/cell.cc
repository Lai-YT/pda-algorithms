#include "cell.h"

#include <cassert>
#include <memory>
#include <vector>

#include "net.h"

using namespace partition;

void Cell::AddNet(std::shared_ptr<Net> net) {
  nets_.push_back(net);
}

void Cell::SetBlock(BlockTag tag) {
  block_tag_ = tag;
  // Give the distribution to each net.
  for (auto& net : nets_) {
    if (block_tag_ == BlockTag::kBlockA) {
      ++net->distribution_.in_a;
    } else {
      ++net->distribution_.in_b;
    }
  }
}

void Cell::MoveTo(BlockTag tag) {
  if (tag == block_tag_) {
    return;
  }
  // Update the distribution of nets.
  for (auto& net : nets_) {
    if (block_tag_ == BlockTag::kBlockA) {
      --net->distribution_.in_a;
      ++net->distribution_.in_b;
    } else {
      --net->distribution_.in_b;
      ++net->distribution_.in_a;
    }
  }
  block_tag_ = tag;
}

bool Cell::Iterator::IsEnd() const {
  return i_ >= cell_.nets_.size();
}

void Cell::Iterator::Next() {
  ++i_;
}

std::shared_ptr<Net> Cell::Iterator::Get() {
  assert(!IsEnd());
  return cell_.nets_.at(i_);
}

Cell::Iterator Cell::GetNetIterator() {
  return Cell::Iterator{*this};
}

bool Cell::IsFree() const {
  return !is_locked_;
}

void Cell::Lock() {
  is_locked_ = true;
}

void Cell::Free() {
  is_locked_ = false;
}
