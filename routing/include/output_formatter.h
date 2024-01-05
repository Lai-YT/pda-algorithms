#ifndef ROUTING_OUTPUT_FORMATTER_H_
#define ROUTING_OUTPUT_FORMATTER_H_

#include <iosfwd>

#include "result.h"

namespace routing {

class OutputFormatter {
 public:
  void Out();

  OutputFormatter(std::ostream& out, Result result)
      : out_{out}, result_{result} {}

 private:
  std::ostream& out_;
  Result result_;
};

}  // namespace routing

#endif  // ROUTING_OUTPUT_FORMATTER_H_
