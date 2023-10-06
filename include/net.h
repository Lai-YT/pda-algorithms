#ifndef PARTITION_NET_H_
#define PARTITION_NET_H_

#include <memory>
#include <vector>

namespace partition {

class Cell;

class Net {
 public:
  /// @brief Connects `cell` with this net.
  void AddCell(std::weak_ptr<Cell> cell);

 private:
  std::vector<std::weak_ptr<Cell>> cells_{};
};
}  // namespace partition

#endif  // PARTITION_NET_H_
