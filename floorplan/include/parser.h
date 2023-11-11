#ifndef FLOORPLAN_PARSER_H_
#define FLOORPLAN_PARSER_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "block.h"

namespace floorplan {

struct Input {
  struct AspectRatio {
    double upper_bound;
    double lower_bound;
  };
  AspectRatio aspect_ratio;
  std::vector<Block> blocks;
};

class Parser {
 public:
  void Parse();

  Input GetInput() const;

  Parser(std::istream& in) : in_{in} {}

 private:
  std::istream& in_;
  Input input_{};

  void ParseAspectRatioConstraint_();
  void ParseBlocks_();
  void ParseBlock_(std::string line);
};
}  // namespace floorplan

#endif  // FLOORPLAN_PARSER_H_
