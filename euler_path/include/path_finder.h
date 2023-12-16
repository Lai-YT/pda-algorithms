#ifndef EULER_PATH_PATH_FINDER_H_
#define EULER_PATH_PATH_FINDER_H_

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace euler {

class Mos;
struct Circuit;
class Net;
struct Path;

using Vertex = std::pair<std::shared_ptr<Mos>, std::shared_ptr<Mos>>;
using Edge = std::pair<std::shared_ptr<Net>, std::shared_ptr<Net>>;
using Neighbors = std::vector<Vertex>;
using Graph = std::map<Vertex, Neighbors>;

class PathFinder {
 public:
  /// @details In addressing the path finder problem, the objective is to
  /// identify an Hamilton path for both P MOS transistors and N MOS
  /// transistors. It is imperative that the paths for these two types of MOS
  /// transistors are identical. To achieve this, we form pairs by grouping a P
  /// MOS transistor with a corresponding N MOS transistor, based on the
  /// commonality of their connections, and subsequently seek an Hamilton path
  /// for each pair.
  /// @return The Hamiltonian path of the MOS, the corresponding net connection,
  /// and the HPWL.
  std::tuple<Path, std::vector<Edge>, double> FindPath();

  PathFinder(const std::shared_ptr<Circuit>& circuit) : circuit_{circuit} {}

 private:
  const std::shared_ptr<Circuit>& circuit_;

  std::map<Vertex, Neighbors> adjacency_list_;
  std::vector<Vertex> vertices_;

  void GroupVertices_();
  void BuildGraph_();

  /// @return The Hamiltonian paths for the graph. The graph may not form a
  /// single path.
  /// @note Our requirement is to only visit each vertex once, while the edges
  /// can be traversed multiple times. This is then in fact a Hamiltonian path
  /// problem: https://en.wikipedia.org/wiki/Hamiltonian_path.
  /// @details Here I'll be using a heuristic algorithm described in the
  /// post: https://mathoverflow.net/a/327893.
  std::vector<Path> FindHamiltonPaths_();
  double CalculateHpwl_(const Path& path) const;

  /// @return The extended Hamiltonian path, if any.
  std::optional<Path> Extend_(Path path, std::set<Vertex>& to_visit) const;
  /// @return The family of the Posa transformations of the given path.
  std::vector<Path> Rotate_(const Path& path) const;
};

}  // namespace euler

#endif  // EULER_PATH_PATH_FINDER_H_
