#ifndef PARTITION_NET_H_
#define PARTITION_NET_H_

#include <cstddef>
#include <memory>
#include <vector>

namespace partition {

class Cell;

class Net {
 public:
  /// @brief Connects `cell` with this net.
  void AddCell(std::weak_ptr<Cell> cell);

  std::size_t Offset() const {
    return offset_;
  }

  /// @param offset Where the net locates in the net array.
  Net(std::size_t offset) : offset_{offset} {}

 private:
  std::vector<std::weak_ptr<Cell>> cells_{};
  std::size_t offset_;
};
}  // namespace partition

#endif  // PARTITION_NET_H_
