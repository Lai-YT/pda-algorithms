#include "util.h"

#include <algorithm>

using namespace routing;

bool routing::IsContainedBy(const Interval& b, const Interval& a) {
  return a.first < b.first && a.second > b.second;
}

bool routing::IsAdjacent(const Interval& a, const Interval& b) {
  return a.first == b.second || a.second == b.first;
}

Interval routing::Union(const Interval& a, const Interval& b) {
  return {std::min(a.first, b.first), std::max(a.second, b.second)};
}
