#ifndef PARTITION_PARSER_H_
#define PARTITION_PARSER_H_

#include <iosfwd>
#include <map>
#include <memory>
#include <string>

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
  std::map<std::string, std::shared_ptr<Net>> GetNetArray() const;
  /// @note Is meaningless if called before `Parse`.
  std::map<std::string, std::shared_ptr<Cell>> GetCellArray() const;

  /// @param in The stream to read characters from.
  Parser(std::istream& in) : in_{in} {}

 private:
  std::istream& in_;

  double balance_factor_ = 0.0;
  std::map<std::string, std::shared_ptr<Net>> net_array_{};
  std::map<std::string, std::shared_ptr<Cell>> cell_array_{};

  std::shared_ptr<Cell> GetCell_(const std::string cell_name);

  void ParseBalanceFactor_();
  void ParseNetConnection();
};

}  // namespace partition

#endif  // PARTITION_PARSER_H_
