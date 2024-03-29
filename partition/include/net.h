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

  /// @return Number of cells this net has in block A.
  std::size_t NumOfCellsInA() const;
  /// @return Number of cells this net has in block B.
  std::size_t NumOfCellsInB() const;

  /// @brief A net is said to be cut if it has at least one cell in each block.
  /// @note This function is possibly expensive. It may iterate over all nets.
  bool IsCut() const;

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

  /// @brief Gets the iterator which is capable of iterate over the cells.
  /// @note Iterator Pattern.
  Iterator GetCellIterator();

 private:
  // For `Cell`s to update the distribution after moving.
  friend class Cell;

  /// @note The cells on the net are store internal to the net itself
  /// instead of in the NET array.
  /// @note Each of these cells is considered a neighbor of the others.
  /// @note Using `weak_ptr` to break the circular referencing between `Cell`
  /// and `Net`.
  std::vector<std::weak_ptr<Cell>> cells_{};

  /// @brief A pair of integers `(A(n), B(n))` which represents the number of
  /// cells the net `n` has in blocks A and B respectively.
  struct Distribution {
    std::size_t in_a;
    std::size_t in_b;
  };
  Distribution distribution_{0, 0};
};
}  // namespace partition

#endif  // PARTITION_NET_H_
