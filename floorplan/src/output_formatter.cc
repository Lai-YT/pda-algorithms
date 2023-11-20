#include "output_formatter.h"

#include <ostream>
#include <string>  // operator

using namespace floorplan;

/// @note The expected format does not allow the end of file newline. Though
/// awkward, it's by intention.
void OutputFormatter::Out() const {
  out_ << "A = " << tree_.Width() * tree_.Height() << '\n';
  out_ << "R = " << tree_.Width() / static_cast<double>(tree_.Height()) << '\n';
  for (auto i = std::size_t{0},
            e = blocks_.size() - 1 /* exclude the last block */;
       i < e; i++) {
    OutBlock_(*blocks_.at(i));
    out_ << '\n';
  }
  OutBlock_(*blocks_.back());
  // No end of file newline.
}

void OutputFormatter::OutBlock_(const Block& block) const {
  out_ << block.name << ' ' << block.bottom_left.x << ' '
       << block.bottom_left.y;
}
