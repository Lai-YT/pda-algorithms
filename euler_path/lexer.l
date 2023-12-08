/*
 * A lexer that recognizes the restricted subset of HSPICE netlist.
 */


/* terminates after a file reaches the end */
%option noyywrap
/* we're not using input & yyunput, don't generate code for them to avoid compiler warnings */
%option noinput
%option nounput
/* automatically track line number */
%option yylineno
/* hspice is case insensitive */
%option caseless

%{

// Give Flex the prototype of yylex we want.
# define YY_DECL \
  yy::parser::symbol_type yylex()

#include <cassert>
#include <cstdlib>
#include <iostream>

#include "mos.h"
#include "y.tab.hh"

// Use LINENO for the actual line number in the source code
// yylineno has advanced to the next line when we reference it.
#define LINENO (yylineno - 1)
%}

MOS_TYPE [pn]mos_rvt
NAME [a-z_][a-z0-9_]*

OP [=]
/* support only nano meter */
UNIT [n]

  /* positive integer or floating point number */
NUMBER [0-9]+(\.[0-9]+)?
%%

  /* keywords */

\.SUBCKT { return yy::parser::make_SUBCKT(); }
\.ENDS { return yy::parser::make_ENDS(); }
  /* Note: several keywords can also be matched as a NAME.
    We have them take the priority. */
{MOS_TYPE} {
  switch (yytext[0]) {
    case 'p':
      return yy::parser::make_MOS_TYPE(euler::Mos::Type::kP);
    case 'n':
      return yy::parser::make_MOS_TYPE(euler::Mos::Type::kN);
    default:
      assert(false && "Invalid MOS type");
  }
}
{UNIT} { return yy::parser::make_UNIT(); }
l { return yy::parser::make_LENGTH(); }
w { return yy::parser::make_WIDTH(); }
nfin { return yy::parser::make_NFIN(); }

{NAME} { return yy::parser::make_NAME(yytext); }
{NUMBER} { return yy::parser::make_NUMBER(std::strtod(yytext, nullptr)); }

{OP} { return yytext[0]; }

  /* special symbols */

<<EOF>> { return yy::parser::make_EOF(); }

[ \t\r] {}

\n { return yy::parser::make_EOL(); }

. {
    std::cerr << "Invalid input: " << yytext << std::endl;
    std::exit(-1);
  }

%%
