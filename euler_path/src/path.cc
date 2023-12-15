#include "path.h"

using namespace euler;

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
