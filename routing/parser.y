%{
#include <iostream>
#include <string>

#include "lex.yy.cc"
%}

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
%token <int> NUMBER

%nterm instance
%nterm boundary boundaries net_ids

%token EOL
%token EOF 0

%%

instance:
  boundaries
  net_ids EOL
  net_ids EOL
  EOF
  ;

boundaries:
  boundary
  | boundaries boundary
  ;

boundary:
  TOP net_ids EOL
  | BOTTOM net_ids EOL
  ;

net_ids:
  NUMBER
  | net_ids NUMBER
  ;


%%

void yy::parser::error(const std::string& err) {
  std::cerr << err << std::endl;
}
