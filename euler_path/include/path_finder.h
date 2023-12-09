#ifndef EULER_PATH_PATH_FINDER_H_
#define EULER_PATH_PATH_FINDER_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace euler {

class Mos;
struct Circuit;

class PathFinder {
 public:
  using Vertex = std::pair<std::shared_ptr<Mos>, std::shared_ptr<Mos>>;

  void FindAPath();

  PathFinder(std::shared_ptr<Circuit> circuit) : circuit_{std::move(circuit)} {}

 private:
  std::shared_ptr<Circuit> circuit_;

  std::vector<Vertex> mos_pairs_{};
  std::map<Vertex, std::vector<Vertex>> graph_{};

  void GroupMosPairs_();
  // O(v^2)
  void BuildGraph_();
};

}  // namespace euler

#endif  // EULER_PATH_PATH_FINDER_H_
