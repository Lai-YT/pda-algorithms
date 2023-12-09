#ifndef EULER_PATH_PATH_FINDER_H_
#define EULER_PATH_PATH_FINDER_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace euler {

class Mos;
struct Circuit;

using Vertex = std::pair<std::shared_ptr<Mos>, std::shared_ptr<Mos>>;
using Neighbors = std::vector<Vertex>;
using EulerPath = std::vector<Vertex>;
using Graph = std::map<Vertex, Neighbors>;

class PathFinder {
 public:
  /// @details In addressing the path finder problem, the objective is to
  /// identify an Euler path for both P MOS transistors and N MOS transistors.
  /// It is imperative that the paths for these two types of MOS transistors are
  /// identical. To achieve this, we form pairs by grouping a P MOS transistor
  /// with a corresponding N MOS transistor, based on the commonality of their
  /// connections, and subsequently seek an Euler path for each pair.
  void FindPath() const;

  PathFinder(const std::shared_ptr<Circuit>& circuit) : circuit_{circuit} {}

 private:
  const std::shared_ptr<Circuit>& circuit_;

  std::vector<Vertex> GroupMosPairs_() const;

  // O(v^2)
  Graph BuildGraph_() const;
};

}  // namespace euler

#endif  // EULER_PATH_PATH_FINDER_H_
