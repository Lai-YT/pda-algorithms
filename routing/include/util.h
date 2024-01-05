#ifndef ROUTING_UTIL_H_
#define ROUTING_UTIL_H_

#include "instance.h"

namespace routing {

/// @return Whether `b` is contained by `a` (proper subset).
bool IsContainedBy(const Interval& b, const Interval& a);

bool IsAdjacent(const Interval& a, const Interval& b);

/// @note The two intervals must be overlapped.
Interval Union(const Interval& a, const Interval& b);

}  // namespace routing

#endif  // ROUTING_UTIL_H_
