#ifndef PARTITION_BUCKET_H_
#define PARTITION_BUCKET_H_

#include <cstddef>
#include <memory>
#include <vector>

namespace partition {

class Cell;

class Bucket {
 public:
  /// @return The largest possible gain.
  std::size_t Pmax() const;

  /// @return The number of cells inside the bucket.
  std::size_t Size() const;

  /// @return The first cell of in the list that the `max_gain` points to.
  /// `nullptr` if the bucket is empty.
  /// @note Since we always only what to get the cell of the maximal gain, it's
  /// unecessary to provide random access on any gains.
  std::shared_ptr<Cell> FirstMaxGainCell() const;

  /// @brief Adds the `cell` to the bucket list with respect to its `gain` and
  /// possibly updates the max gain.
  void Add(std::shared_ptr<Cell> cell);
  /// @brief Removes the `cell` from the bucket list with respect to its
  /// `gain` and possibly updates the max gain.
  void Remove(std::shared_ptr<Cell> cell);

  Bucket(std::size_t pmax = 0)
      : pmax_{pmax},
        list_{pmax * 2 + 1 /* -pmax ~ pmax */, nullptr},
        max_gain_{static_cast<int>(-pmax)} {}

 private:
  /// @brief The offset to map the gain to the index.
  std::size_t pmax_;
  /// @brief The bucket list to track the gains.
  /// @note The `Cell` itself is a doubly linked list.
  std::vector<std::shared_ptr<Cell>> list_;
  /// @brief The number of cells inside the bucket. They should all be free.
  std::size_t size_{0};
  /// @brief Keep track of the bucket having the a cell of highest gain.
  int max_gain_;

  /// @param gain -pmax <= gain <= pmax
  /// @note Maps the gain to the index using the offset.
  std::size_t ToIndex_(int gain) const;
};
}  // namespace partition

#endif  // PARTITION_BUCKET_H_
