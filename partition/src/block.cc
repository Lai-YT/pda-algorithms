#include "block.h"

#include "cell.h"

using namespace partition;

void Block::Add(std::shared_ptr<Cell> cell) {
  Add(cell->Offset());
}

void Block::Add(std::size_t cell) {
  cells_.insert(cell);
}

void Block::Remove(std::shared_ptr<Cell> cell) {
  Remove(cell->Offset());
}

void Block::Remove(std::size_t cell) {
  cells_.erase(cell);
}

bool Block::Contains(std::shared_ptr<Cell> cell) const {
  return Contains(cell->Offset());
}

bool Block::Contains(std::size_t cell) const {
  return cells_.find(cell) != cells_.cend();
}
