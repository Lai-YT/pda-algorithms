#include <cstdio>
#include <fstream>
#include <string>

#include "arg.h"
#include "instance.h"
#include "output_formatter.h"
#include "result.h"
#include "router.h"
#include "y.tab.hh"

#ifdef DEBUG
#include <iostream>
#endif

extern FILE* yyin;
extern void yylex_destroy();

using namespace routing;

Instance instance;

int main(int argc, char* argv[]) {
  auto arg = HandleArguments(argc, argv);
  auto in = fopen(arg.in.c_str(), "r");
  if (!(yyin = in)) {
    std::perror(argv[1]);
    return 1;
  }
  yy::parser parser{};
  int ret = parser.parse();

  yylex_destroy();
  fclose(in);

  // 0 on success, 1 otherwise
  if (ret) {
    return ret;
  }

#ifdef DEBUG
  std::cout << "TOP\n";
  for (auto i = std::size_t{0}; i < instance.top_boundaries.size(); i++) {
    std::cout << i << ": ";
    for (auto interval : instance.top_boundaries.at(i)) {
      std::cout << "(" << interval.first << ", " << interval.second << ")"
                << " ";
    }
    std::cout << '\n';
  }
  std::cout << "BOTTOM\n";
  for (auto i = std::size_t{0}; i < instance.bottom_boundaries.size(); i++) {
    std::cout << i << ": ";
    for (auto interval : instance.bottom_boundaries.at(i)) {
      std::cout << "(" << interval.first << ", " << interval.second << ")"
                << " ";
    }
    std::cout << '\n';
  }
  std::cout << "TOP NETS\n";
  for (auto net_idx : instance.top_net_ids) {
    std::cout << net_idx << " ";
  }
  std::cout << '\n';
  std::cout << "BOTTOM NETS\n";
  for (auto net_idx : instance.bottom_net_ids) {
    std::cout << net_idx << " ";
  }
  std::cout << '\n';
#endif

  auto router = Router{instance};
  auto result = router.Route();

  auto out = std::ofstream{arg.out};
  auto output_formatter = OutputFormatter{out, result};
  output_formatter.Out();

  return 0;
}
