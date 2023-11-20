#include "parser.h"

#include <limits>
#include <sstream>
#include <string>
#include <utility>  // move

#include "block.h"

using namespace floorplan;

void Parser::Parse() {
  ParseAspectRatioConstraint_();
  ParseBlocks_();
}

Input Parser::GetInput() const {
  return input_;
}

void Parser::ParseAspectRatioConstraint_() {
  in_ >> input_.aspect_ratio.lower_bound >> input_.aspect_ratio.upper_bound;
  // cleanly remove this line form the input stream
  in_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void Parser::ParseBlocks_() {
  for (auto line = std::string{}; std::getline(in_, line); /* empty */) {
    ParseBlock_(std::move(line));
  }
}

void Parser::ParseBlock_(std::string line) {
  auto ss = std::stringstream{std::move(line), std::ios_base::in};
  auto block = Block{};
  ss >> block.name >> block.width >> block.height;
  input_.blocks.push_back(std::move(block));
}
