#include "cell.h"

#include <memory>
#include <vector>

using namespace partition;

void Cell::AddNet(std::shared_ptr<Net> net) {
  nets_.push_back(net);
}
