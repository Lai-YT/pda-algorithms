#include "y.tab.hh"

extern void yylex_destroy();

int main(int argc, char const* argv[]) {
  yy::parser parser{};
  int ret = parser.parse();

  yylex_destroy();

  // 0 on success, 1 otherwise
  if (ret) {
    return ret;
  }

  return 0;
}
