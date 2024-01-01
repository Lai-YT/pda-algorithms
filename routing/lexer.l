/* terminates after a file reaches the end */
%option noyywrap
/* we're not using input & yyunput, don't generate code for them to avoid compiler warnings */
%option noinput
%option nounput
/* automatically track line number */
%option yylineno

%{

// Give Flex the prototype of yylex we want.
# define YY_DECL \
  yy::parser::symbol_type yylex()

#include <string>
#include <iostream>

#include "y.tab.hh"

// Use LINENO for the actual line number in the source code
// yylineno has advanced to the next line when we reference it.
#define LINENO (yylineno - 1)
%}

NUMBER ([1-9][0-9]*)|0
TOP T{NUMBER}
BOTTOM B{NUMBER}

%%

  /* keywords */

{TOP} { return yy::parser::make_TOP(std::stoi(yytext + 1 /* discard the T */)); }
{BOTTOM} { return yy::parser::make_BOTTOM(std::stoi(yytext + 1 /* discard the B */)); }

  /* numbers */

{NUMBER} { return yy::parser::make_NUMBER(std::stoi(yytext)); }

  /* special symbols */

<<EOF>> { return yy::parser::make_EOF(); }

[ \t\r] {}
\n { return yy::parser::make_EOL(); }

  /* error handling */

. {
    std::cerr << "Invalid input: " << yytext << std::endl;
    std::exit(-1);
  }

%%
