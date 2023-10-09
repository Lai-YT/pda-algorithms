#include "cell.h"

#include <cassert>
#include <memory>
#include <vector>

using namespace partition;

void Cell::AddNet(std::shared_ptr<Net> net) {
  nets_.push_back(net);
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
