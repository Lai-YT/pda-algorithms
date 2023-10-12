#ifndef PARTITION_FM_PARTITIONER_H_
#define PARTITION_FM_PARTITIONER_H_

#include <cstddef>
#include <memory>
#include <vector>

#include "block.h"
#include "block_tag.h"

namespace partition {

class Cell;
class Net;

/// @brief Partitions the cells into 2 blocks. The goal is to obtain minimum cut
/// size while adhering to the constraint of the balance factor.
class FmPartitioner {
 public:
  FmPartitioner(double balance_factor,
                std::vector<std::shared_ptr<Cell>> cell_array,
                std::vector<std::shared_ptr<Net>> net_array);

  void Partition();

 private:
  double balance_factor_;
  std::vector<std::shared_ptr<Cell>> cell_arr_;
  std::vector<std::shared_ptr<Net>> net_arr_;
  Block a_{BlockTag::kBlockA};
  Block b_{BlockTag::kBlockB};

  struct Bucket_ {
    /// @brief The bucket list to track the gains.
    /// @note The `Cell` itself is a doubly linked list.
    std::vector<std::shared_ptr<Cell>> list;
    /// @note This has to be manually maintain once a cell is added or removed
    /// from the list.
    std::size_t size;
    /// @brief Keep track of the bucket having the a cell of highest gain.
    int max_gain;
    /// @brief The offset to map the gain to the index.
    int pmax;

    /// @param gain -pmax <= gain <= pmax
    /// @note Maps the gain to the index using the offset.
    std::size_t ToIndex(int gain) const;
  };
  Bucket_ bucket_a_{};
  Bucket_ bucket_b_{};

  /// @brief Generates the initial partition randomly.
  void InitPartition_();

  /// @brief Calculates the distribution of each nets with respect to the
  /// partition of the cells.
  void CalculateDistribution_();

  /// @brief Calculates the gains of each cells with respect to the initial
  /// partition and builds up the linked list structure between the cells.
  void CalculateCellGains_();

  /// @return Number of cells `net` has on the From side of `cell`.
  std::size_t& F(std::shared_ptr<Cell> cell, std::shared_ptr<Net> net) const;
  /// @return Number of cells `net` has on the To side of `cell`.
  std::size_t& T(std::shared_ptr<Cell> cell, std::shared_ptr<Net> net) const;

  void RunPass_();

  std::shared_ptr<Cell> ChooseBaseCell_() const;

  void UpdateCellToGain_(std::shared_ptr<Cell> cell, int gain);

  /// @brief Removes the `cell` from the bucket list with respect to its `gain`
  /// and possibly updates the max gain.
  void RemoveCellFromBucket_(std::shared_ptr<Cell> cell);
  /// @brief Adds the `cell` to the bucket list with respect to its `gain` and
  /// possibly updates the max gain.
  void AddCellToBucket_(std::shared_ptr<Cell> cell);

  Bucket_& GetBucket_(std::shared_ptr<Cell> cell);

  /// @note Since the size of all cells are fixed to 1, it doesn't have to be
  /// passed.
  bool IsBalancedAfterMoving_(const Block& from, const Block& to) const;

  struct Record_ {
    int gain;
    std::shared_ptr<Cell> cell;
    /// @brief Whether the partitioning is balanced after this move.
    bool is_balanced;
  };
  /// @brief All moves are recorded in the history. After a single pass, we'll
  /// go through the history and restore the state that has the minimal cut
  /// size.
  std::vector<Record_> history_{};

  /// @return To which move do we obtain the maximum gain. It's an index of the
  /// `history`. `-1` if no positive gain.
  /// @note Max gain is equivalent to minimum cut size.
  int FindPartitionOfMaxPositiveGainFromHistory_() const;
  /// @brief Reverts all moves starting from the one at index `idx` of the
  /// `history`.
  void RevertAllMovesAfter_(std::size_t idx);
};

}  // namespace partition

#endif  // PARTITION_FM_PARTITIONER_H_
