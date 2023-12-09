#include "circuit.h"

using namespace euler;

#include <memory>
#include <vector>

void Net::AddConnection(std::weak_ptr<Mos> mos) {
  // FIXME: Linear search is slow. Use another data structure for existence
  // checking.
  for (const auto& m : mos_) {
    if (m.lock() == mos.lock()) {
      return;
    }
  }
  mos_.push_back(mos);
}
