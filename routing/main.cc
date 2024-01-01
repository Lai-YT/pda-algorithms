#include <cstdio>
#include <fstream>
#include <string>
#include "arg.h"
#include "y.tab.hh"

extern FILE* yyin;
extern void yylex_destroy();

using namespace routing;

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

  auto out = std::ofstream{arg.out};

  return 0;
}
