#include "net.h"

#include <cassert>
#include <memory>
#include <vector>

using namespace partition;

void Net::AddCell(std::weak_ptr<Cell> cell) {
  cells_.push_back(cell);
}

bool Net::Iterator::IsEnd() const {
  return i_ >= net_.cells_.size();
}

void Net::Iterator::Next() {
  ++i_;
}

std::shared_ptr<Cell> Net::Iterator::Get() {
  assert(!IsEnd());
  return net_.cells_.at(i_).lock();
}

Net::Iterator Net::GetCellIterator() {
  return Net::Iterator{*this};
}
