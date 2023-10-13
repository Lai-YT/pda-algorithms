#include "parser.h"

#include <cstddef>
#include <istream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cell.h"
#include "net.h"

using namespace partition;

void Parser::Parse() {
  ParseBalanceFactor_();
  while (in_) {
    ParseNetConnection_();
  }
}

void Parser::ParseBalanceFactor_() {
  in_ >> balance_factor_;
}

void Parser::ParseNetConnection_() {
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
  auto net = std::make_shared<Net>(net_arr_.size());
  net_arr_.push_back(net);

  // Data cleaning; avoid duplicate cells in a single net.
  auto cells_already_in_this_connection = std::unordered_set<std::size_t>{};
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
    auto cell = cell_arr_.at(GetOffsetOfCell_(cell_name));
    if (!cells_already_in_this_connection.count(cell->Offset())) {
      net->AddCell(cell);
      cell->AddNet(net);
      cells_already_in_this_connection.insert(cell->Offset());
    }
  }
}

double Parser::GetBalanceFactor() const {
  return balance_factor_;
}

std::size_t Parser::GetOffsetOfCell_(const std::string& cell_name) {
  if (offset_of_cell_.count(cell_name)) {
    return offset_of_cell_.at(cell_name);
  }
  auto cell = std::make_shared<Cell>(cell_arr_.size());
  cell->name = cell_name;
  cell_arr_.push_back(cell);
  offset_of_cell_[cell_name] = cell->Offset();
  return cell->Offset();
}

std::vector<std::shared_ptr<Net>> Parser::GetNetArray() const {
  // TODO: coping the entire map is slow
  return net_arr_;
}

std::vector<std::shared_ptr<Cell>> Parser::GetCellArray() const {
  // TODO: coping the entire map is slow
  return cell_arr_;
}
