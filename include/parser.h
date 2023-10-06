#ifndef PARTITION_PARSER_H_
#define PARTITION_PARSER_H_

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace partition {

class Cell;
class Net;

class Parser {
 public:
  /// @note The input is assumed to have valid format. Erroneous input may crash
  /// the program.
  void Parse();

  /// @note Is meaningless if called before `Parse`.
  double GetBalanceFactor() const;
  /// @note Is meaningless if called before `Parse`.
  std::vector<std::shared_ptr<Net>> GetNetArray() const;
  /// @note Is meaningless if called before `Parse`.
  std::vector<std::shared_ptr<Cell>> GetCellArray() const;

  /// @param in The stream to read characters from.
  Parser(std::istream& in) : in_{in} {}

 private:
  std::istream& in_;

  double balance_factor_ = 0.0;

  /// @brief Since a single cell may appear multiple times during parsing, an
  /// addition data structure is used to check whether it has already been
  /// constructed, and to locate the constructed cell from the array.
  std::unordered_map<std::string, std::size_t> offset_of_cell_{};

  std::vector<std::shared_ptr<Net>> net_array_{};
  std::vector<std::shared_ptr<Cell>> cell_array_{};

  std::size_t GetOffsetOfCell_(const std::string cell_name);

  void ParseBalanceFactor_();
  void ParseNetConnection();
};

}  // namespace partition

#endif  // PARTITION_PARSER_H_
