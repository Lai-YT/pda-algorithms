#include "block.h"

#include <memory>

using namespace partition;

void Block::Add(std::shared_ptr<Cell>) {
  ++size_;
}

void Block::Remove(std::shared_ptr<Cell>) {
  --size_;
}
