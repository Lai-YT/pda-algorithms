/*
 * A parser that handles the restricted subset of HSPICE netlist.
 */

%{

#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "lex.yy.cc"
#include "circuit.h"

extern std::unique_ptr<euler::Circuit> circuit;

static std::map<std::string, std::shared_ptr<euler::Net>> nets;

static std::shared_ptr<euler::Net> GetOrCreateNet(const std::string& name);
static std::shared_ptr<euler::Net> GetNetOrNull(const std::string& name);
static void RegisterNet(std::shared_ptr<euler::Net> net);
%}

// Dependency code required for the value and location types;
// inserts verbatim to the header file.
%code requires {
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "circuit.h"
#include "mos.h"
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

/* keywords */
%token SUBCKT ENDS
%token <euler::Mos::Type> MOS_TYPE
%token UNIT
%token LENGTH WIDTH NFIN
%token <std::string> NAME
%token <double> NUMBER

%nterm circuit
%nterm <std::shared_ptr<euler::Mos>>mos
%nterm <std::vector<std::shared_ptr<euler::Mos>>> mos_list
%nterm net_list

%token EOL
%token EOF 0

%%

circuit:
  SUBCKT NAME net_list EOL
  mos_list
  ENDS {
    circuit = std::make_unique<euler::Circuit>($5, std::move(nets));
  }
  ;

net_list:
  net_list NAME {
    auto net = std::make_shared<euler::Net>(std::move($2));
    RegisterNet(net);
  }
  | NAME {
    auto net = std::make_shared<euler::Net>(std::move($1));
    RegisterNet(net);
  }
  ;

mos_list:
  mos_list mos EOL {
    $$ = std::move($1);
    $$.push_back($2);
  }
  | mos EOL {
    $$ = std::vector<std::shared_ptr<euler::Mos>>{$1};
  }
  ;

mos:
  NAME NAME NAME NAME NAME MOS_TYPE WIDTH '=' NUMBER UNIT LENGTH '=' NUMBER UNIT NFIN '=' NUMBER {
    $$ = euler::Mos::Create(/* name */ $1, /* type */ $6, GetOrCreateNet($2),
                            GetOrCreateNet($3), GetOrCreateNet($4), GetOrCreateNet($5));
    $$->RegisterToConnections();
  }
  ;

%%

void yy::parser::error(const std::string& err) {
  std::cerr << err << std::endl;
}

static std::shared_ptr<euler::Net> GetOrCreateNet(const std::string& name) {
    auto net = GetNetOrNull(name);
    if (!net) {
        net = std::make_shared<euler::Net>(name);
        RegisterNet(net);
    }
    return net;
}

static std::shared_ptr<euler::Net> GetNetOrNull(const std::string& name) {
  if (auto net = nets.find(name); net == nets.cend()) {
    return nullptr;
  } else {
    return net->second;
  }
}

static void RegisterNet(std::shared_ptr<euler::Net> net) {
  nets.emplace(net->GetName(), net);
}
