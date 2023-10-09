#ifndef PARTITION_NET_H_
#define PARTITION_NET_H_

#include <cstddef>
#include <memory>
#include <vector>

namespace partition {

class Cell;

class Net {
 public:
  /// @brief Places the `cell` on this net.
  void AddCell(std::weak_ptr<Cell> cell);

  std::size_t Offset() const {
    return offset_;
  }

  /// @param offset Where the net locates in the net array.
  Net(std::size_t offset) : offset_{offset} {}

 private:
  /// @note The cells on the net are store internal to the net itself
  /// instead of in the NET array.
  /// @note Each of these cells is considered a neighbor of the others.
  /// @note Using weak_ptr to break the circular referencing between `Cell` and
  /// `Net`.
  std::vector<std::weak_ptr<Cell>> cells_{};
  std::size_t offset_;
};
}  // namespace partition

#endif  // PARTITION_NET_H_
