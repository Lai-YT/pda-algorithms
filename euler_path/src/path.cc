#include "path.h"

#ifdef DEBUG
#include <iostream>

#include "circuit.h"
#include "mos.h"
#endif

namespace euler {

Path::Path(const Path& other) {
  *this = other;
}

Path& Path::operator=(const Path& other) {
  if (this == &other) {
    return *this;
  }

  auto this_prev = std::shared_ptr<PathFragment>{};
  auto this_curr = std::shared_ptr<PathFragment>{};
  for (auto other_curr = other.head; other_curr;
       other_curr = other_curr->next) {
    this_curr = std::make_shared<PathFragment>(
        other_curr->vertex, this_prev, nullptr, other_curr->edge_to_next);
    if (this_prev) {
      this_prev->next = this_curr;
    } else {
      head = this_curr;
    }
    this_prev = this_curr;
  }
  tail = this_curr;
  return *this;
}

#ifdef DEBUG
void PrintPath(const Path& path) {
  for (auto curr = path.head; curr; curr = curr->next) {
    std::cerr << "[V] " << curr->vertex.first->GetName() << " ";
    if (curr->next) {
      std::cerr << "[E] " << curr->edge_to_next.first->GetName() << " ";
    }
  }
  std::cerr << std::endl;
  for (auto curr = path.head; curr; curr = curr->next) {
    std::cerr << "[V] " << curr->vertex.second->GetName() << " ";
    if (curr->next) {
      std::cerr << "[E] " << curr->edge_to_next.second->GetName() << " ";
    }
  }
  std::cerr << std::endl;
}
#endif

}  // namespace euler
