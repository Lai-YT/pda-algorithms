#ifndef PARTITION_FM_PARTITIONER_H_
#define PARTITION_FM_PARTITIONER_H_

#include <cstddef>
#include <memory>
#include <vector>
#include <utility>

#include "block.h"
#include "block_tag.h"
#include "bucket.h"

namespace partition {

class Cell;
class Net;

/// @brief Partitions the cells into 2 blocks using the Fiduccia-Mattheyses
/// algorithm. The goal is to obtain minimum cut size while adhering to the
/// constraint of the balance factor.
class FmPartitioner {
 public:
  void Partition();

  /// @note Is meaningless if called before `Partition`.
  std::size_t GetCutSize() const;
  /// @note Is meaningless if called before `Partition`.
  std::vector<std::shared_ptr<Cell>> GetBlockA() const;
  /// @note Is meaningless if called before `Partition`.
  std::vector<std::shared_ptr<Cell>> GetBlockB() const;

  FmPartitioner(double balance_factor,
                std::vector<std::shared_ptr<Cell>> cell_array,
                std::vector<std::shared_ptr<Net>> net_array);

 private:
  double balance_factor_;
  std::vector<std::shared_ptr<Cell>> cell_arr_;
  std::vector<std::shared_ptr<Net>> net_arr_;
  Block a_{BlockTag::kBlockA};
  Block b_{BlockTag::kBlockB};
  Bucket bucket_a_{};
  Bucket bucket_b_{};

  /// @brief Generates the initial partition randomly.
  void InitPartition_();

  /// @brief Calculates the gains of each cells with respect to the initial
  /// partition and builds up the bucket list structure between the cells.
  void CalculateCellGains_();

  /// @return Number of cells `net` has on the From side of `cell`.
  std::size_t F(std::shared_ptr<Cell> cell, std::shared_ptr<Net> net) const;
  /// @return Number of cells `net` has on the To side of `cell`.
  std::size_t T(std::shared_ptr<Cell> cell, std::shared_ptr<Net> net) const;

  /// @brief Runs a single pass of partition, which moves the cells and fills up
  /// the history.
  void RunPass_();

  /// @brief Chooses the next cell to be moved.
  /// @return `nullptr` if all cells have been tried.
  std::shared_ptr<Cell> ChooseBaseCell_() const;

  /// @brief Moves `cell` to the list with `gain` in the bucket it belongs to.
  void UpdateCellToGain_(std::shared_ptr<Cell> cell, int gain);

  /// @return The bucket of the block which `cell` is in.
  Bucket& GetBucketOf_(std::shared_ptr<Cell> cell);

  /// @note Since the size of all cells are fixed to 1, it doesn't have to be
  /// passed.
  bool IsBalancedAfterMoving_(const Block& from, const Block& to) const;
  /// @param s Either the size of block A or block B.
  /// @note Since the two blocks are complementary, we don't need to check on
  /// both.
  bool IsBalanced_(std::size_t) const;

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

  /// @return To which move do we obtain the maximum balanced gain, which is an
  /// index of the `history`, and the maximum balanced gain that we obtained.
  /// @note Max gain is equivalent to minimum cut size.
  std::pair<std::size_t, int> FindIdxOfMaxBalancedGainFromHistory_() const;
  /// @brief Reverts all moves starting from the one at index `idx` of the
  /// `history`.
  void RevertAllMovesAfter_(std::size_t idx);
};

}  // namespace partition

#endif  // PARTITION_FM_PARTITIONER_H_
