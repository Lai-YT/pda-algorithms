TARGET := Lab3
CXX := g++
CC = $(CXX)
CXXFLAGS = -g3 -std=c++17 -Wall -MMD -Iinclude
CFLAGS = $(CXXFLAGS)
LEX = lex
# C++ features are used, yacc doesn't suffice
YACC = bison
# -d: generate header with default name
YFLAGS = --debug -d

# Note that lex.yy.cpp is excluded deliberately, as "lex.yy.cc" is considered a
# header file (it's included by "y.tab.cc").
OBJS := $(shell find . -name "*.cc" ! -name "y.tab.cc" ! -name "lex.yy.cc" ) y.tab.o
OBJS := $(OBJS:.cc=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

lex.yy.cc: lexer.l
	$(LEX) -o $@ $<

y.tab.hh: y.tab.cc
y.tab.cc: parser.y lex.yy.cc
	$(YACC) $(YFLAGS) $< -o $@

#
# Please note that although we're handling dependencies automatically with -MMD,
# files that includes Flex or Bison-generated files still have to make such
# dependency explicit to enforce the ordering.
#

main.o: %.o: %.cc y.tab.hh

clean:
	rm -rf *.o lex.yy.* y.tab.* $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
