#include "circuit.h"

using namespace euler;

#include <memory>
#include <vector>

void Net::AddConnection(std::weak_ptr<Mos> mos) {
  mos_.push_back(mos);
}
