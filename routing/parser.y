%{
#include <iostream>
#include <string>

#include "instance.h"
#include "lex.yy.cc"

extern Instance instance;
%}

%code requires {
  #include <array>
  #include <tuple>
  #include <utility>
  #include <vector>

  #include "instance.h"

  using namespace routing;
}

%skeleton "lalr1.cc"
%require "3.2"
%language "c++"

// Use complete symbols (parser::symbol_type).
%define api.token.constructor
// Allow non-pointer-based rich types.
%define api.value.type variant
// Check whether symbols are constructed and destructed using RTTI.
%define parse.assert
// This guarantees that headers do not conflict when included together.
%define api.token.prefix {TOK_}
// Have messages report the unexpected token, and possibly the expected ones.
// Without this, the error message is always only "syntax error".
%define parse.error verbose
// Improve syntax error handling, as LALR parser might perform additional
// parser stack reductions before discovering the syntax error.
%define parse.lac full

%token <int> TOP BOTTOM
%token <unsigned> POS_NUMBER

%nterm instance
%nterm <std::tuple<BoundaryKind, unsigned /* the distance from the innermost boundary */, Interval>> boundary
  /* Notice that there may be multiple intervals having the same distance from the innermost boundary. */
%nterm <std::array<std::vector<std::vector<Interval>>, 2 /* top, bottom */>> boundaries
%nterm <Interval> interval
%nterm <NetIds> net_ids

%token EOL
%token EOF 0

%%

instance:
  boundaries
  net_ids EOL
  net_ids EOL
  EOF {
    instance = Instance{
      .top_boundaries = $1.at(BoundaryKind::kTop),
      .bottom_boundaries = $1.at(BoundaryKind::kBottom),
      .top_net_ids = $2,
      .bottom_net_ids = $4,
    };
  }
  ;

boundaries:
  boundary {
    auto boundaries = std::array<std::vector<std::vector<Interval>>, 2>{};
    auto [kind, dist, new_interval] = $1;
    if (dist >= boundaries.at(kind).size()) {
      boundaries.at(kind).resize(dist + 1);
    }
    // These nets goes to the same boundary, append them.
    auto& boundary_with_dist = boundaries.at(kind).at(dist);
    boundary_with_dist.push_back(new_interval);
    $$ = boundaries;
  }
  | boundaries boundary {
    auto boundaries = $1;
    auto [kind, dist, new_interval] = $2;
    if (dist >= boundaries.at(kind).size()) {
      boundaries.at(kind).resize(dist + 1);
    }
    // These nets goes to the same boundary, append them.
    auto& boundary_with_dist = boundaries.at(kind).at(dist);
    boundary_with_dist.push_back(new_interval);
    $$ = boundaries;
  }
  ;

boundary:
  TOP interval EOL {
    $$ = std::make_tuple(BoundaryKind::kTop, $1, $2);
  }
  | BOTTOM interval EOL {
    $$ = std::make_tuple(BoundaryKind::kBottom, $1, $2);
  }
  ;

net_ids:
  POS_NUMBER {
    $$ = NetIds{$1};
  }
  | net_ids POS_NUMBER {
    $1.push_back($2);
    $$ = $1;
  }
  ;

interval:
  POS_NUMBER POS_NUMBER {
    $$ = Interval{$1, $2};
  }
  ;


%%

void yy::parser::error(const std::string& err) {
  std::cerr << err << std::endl;
}
