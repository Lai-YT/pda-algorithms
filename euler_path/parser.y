/*
 * A parser that handles the restricted subset of HSPICE netlist.
 */

%{

#include <iostream>
#include <string>

#include "lex.yy.cc"

%}

// Dependency code required for the value and location types;
// inserts verbatim to the header file.
%code requires {
#include <string>

#include "type.h"
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
// Copy may be expensive when using rich types, such as std::vector.
// Also with automove, smart pointers can be moved implicitly without boilerplate std::move.
// NOTE: can no longer reference a $x twice since it's moved in the first place.
/* %define api.value.automove */
// This guarantees that headers do not conflict when included together.
%define api.token.prefix {TOK_}
// Have messages report the unexpected token, and possibly the expected ones.
// Without this, the error message is always only "syntax error".
%define parse.error verbose
// Improve syntax error handling, as LALR parser might perform additional
// parser stack reductions before discovering the syntax error.
%define parse.lac full

/* keywords */
%token SUBCKT ENDS
%token <euler::MosType> MOS_TYPE
%token UNIT
%token LENGTH WIDTH NFIN
%token <std::string> NAME
%token <double> NUMBER

%nterm mos mos_list pin pin_list

%token EOL
%token EOF 0

%%

circuit:
  SUBCKT NAME pin_list EOL
  mos_list
  ENDS
  ;

pin_list:
  pin_list pin
  | pin
  ;

mos_list:
  mos_list mos EOL
  | mos EOL
  ;

mos:
  /* (M)instance-name drain gate source substrate type width length nfin */
  NAME pin pin pin pin MOS_TYPE WIDTH '=' NUMBER UNIT LENGTH '=' NUMBER UNIT NFIN '=' NUMBER
  ;

pin:
  NAME
  ;
%%

void yy::parser::error(const std::string& err) {
  std::cerr << err << std::endl;
}
