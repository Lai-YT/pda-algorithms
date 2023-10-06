#include "net.h"

#include <memory>
#include <vector>

using namespace partition;

void Net::AddCell(std::weak_ptr<Cell> cell) {
  cells_.push_back(cell);
}
