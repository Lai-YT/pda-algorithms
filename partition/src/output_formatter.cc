#include "output_formatter.h"

#include <ostream>

#include "cell.h"

using namespace partition;

void OutputFormatter::Out() const {
  out_ << "Cutsize = " << cut_size_ << '\n';
  out_ << "G1 " << block_a_.size() << '\n';
  for (const auto& cell : block_a_) {
    out_ << cell->name << ' ';
  }
  out_ << ";\n";
  out_ << "G2 " << block_b_.size() << '\n';
  for (const auto& cell : block_b_) {
    out_ << cell->name << ' ';
  }
  out_ << ";\n";
}
