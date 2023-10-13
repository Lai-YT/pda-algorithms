#ifndef PARTITION_NET_H_
#define PARTITION_NET_H_

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace partition {

class Cell;

class Net {
 public:
  /// @brief Places the `cell` on this net.
  void AddCell(std::weak_ptr<Cell> cell);

  class Iterator {
   public:
    bool IsEnd() const;
    /// @brief Advances the iterator.
    void Next();
    std::shared_ptr<Cell> Get();

    /// @param net The net that has cells to iterate over.
    Iterator(Net& net) : net_{net} {}

   private:
    Net& net_;
    std::size_t i_{0};
  };

  /// @note Iterator Pattern
  Iterator GetCellIterator();

  std::size_t Offset() const {
    return offset_;
  }

  /// @brief An ordered pair of integers `(A(n), B(n))` which represents the
  /// number of cells the net `n` has in blocks A and B respectively.
  std::pair<std::size_t, std::size_t> distribution;
  /// @brief A net is said to be cut if it has at least one cell in each block.
  /// @note This function is possibly expensive. It may iterate over all nets.
  bool IsCut() const;

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