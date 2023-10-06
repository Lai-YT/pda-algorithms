#include "parser.h"

#include <cstddef>
#include <istream>
#include <string>
#include <unordered_map>

#include "cell.h"
#include "net.h"

using namespace partition;

void Parser::Parse() {
  ParseBalanceFactor_();
  while (in_) {
    ParseNetConnection();
  }
}

void Parser::ParseBalanceFactor_() {
  in_ >> balance_factor_;
}

void Parser::ParseNetConnection() {
  // A single input line of the connection of a net has the following format:
  // NET <Net Name> [<Cell Name>]+;

  auto keyword_net = std::string{};
  if (!(in_ >> keyword_net)) {
    // End of file.
    return;
  }

  auto net_name = std::string{};
  in_ >> net_name;
  // Each net only appears once in the input, so this must be the first time
  // we see this net. Construct it.
  auto net = std::make_shared<Net>(net_array_.size());
  net_array_.push_back(net);

  auto cell_name = std::string{};
  bool saw_delimiter = false;
  while (!saw_delimiter && in_ >> cell_name) {
    // The delimiter may or may not stick with the last cell name.
    if (cell_name == ";") {
      break;
    }
    if (cell_name.back() == ';') {
      cell_name.pop_back();
      saw_delimiter = true;
    }

    auto cell = cell_array_.at(GetOffsetOfCell_(cell_name));
    net->AddCell(cell);
    cell->AddNet(net);
  }
}

double Parser::GetBalanceFactor() const {
  return balance_factor_;
}

std::size_t Parser::GetOffsetOfCell_(const std::string cell_name) {
  if (offset_of_cell_.count(cell_name)) {
    return offset_of_cell_.at(cell_name);
  }
  auto cell = std::make_shared<Cell>(cell_array_.size());
  cell_array_.push_back(cell);
  offset_of_cell_[cell_name] = cell->Offset();
  return cell->Offset();
}

std::vector<std::shared_ptr<Net>> Parser::GetNetArray() const {
  // TODO: coping the entire map is slow
  return net_array_;
}

std::vector<std::shared_ptr<Cell>> Parser::GetCellArray() const {
  // TODO: coping the entire map is slow
  return cell_array_;
}
