#ifndef EULER_PATH_PATH_H_
#define EULER_PATH_PATH_H_

#include <memory>
#include <utility>
#include <vector>

#include "path_finder.h"

namespace euler {

struct PathFragment {
  PathFragment(Vertex vertex, std::weak_ptr<PathFragment> prev,
               std::shared_ptr<PathFragment> next, Edge edge_with_next)
      : vertex{std::move(vertex)},
        prev{prev},
        next{next},
        edge_to_next{std::move(edge_with_next)} {}

  PathFragment(Vertex vertex, std::weak_ptr<PathFragment> prev)
      : PathFragment{vertex, prev, nullptr, {nullptr, nullptr}} {}

  PathFragment(Vertex vertex)
      : PathFragment{vertex, {}, nullptr, {nullptr, nullptr}} {}

  Vertex vertex;
  std::weak_ptr<PathFragment> prev;
  std::shared_ptr<PathFragment> next;

  // Records only the edge used to connect to `next`, as the edge used to
  // connect to `prev` can be retrieved from `prev`.

  Edge edge_to_next;
};

struct Path {
  std::shared_ptr<PathFragment> head;
  std::shared_ptr<PathFragment> tail;

  Path() = default;
  /// @brief Deep copy.
  Path(const Path& other);
  /// @brief Deep copy.
  Path& operator=(const Path& other);
  Path(Path&& other) noexcept = default;
  Path& operator=(Path&& other) noexcept = default;
};

}  // namespace euler

#endif  // EULER_PATH_PATH_H_
