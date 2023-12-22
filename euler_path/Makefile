TARGET := EulerPath
CXX := g++
CC = $(CXX)
CXXFLAGS = -g3 -std=c++17 -Wall -MMD -Iinclude
CFLAGS = $(CXXFLAGS)
LEX = flex
# C++ features are used, yacc doesn't suffice
YACC = bison
# -d: generate header with default name
YFLAGS = --debug -d

# Note that lex.yy.cpp is excluded deliberately, as "lex.yy.cc" is considered a
# header file (it's included by "y.tab.cc").
OBJS := $(shell find . -name "*.cc" ! -name "y.tab.cc" ! -name "lex.yy.cc" ) y.tab.o
OBJS := $(OBJS:.cc=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean release debug assertion profile help iwyu lexer parser

all: $(TARGET)

$(TARGET): release

# no assertion or debug, fully optimized
release: CXXFLAGS += -O3 -DNDEBUG
# assertion + state printing, few optimized
debug: CXXFLAGS += -O0 -g3 -DDEBUG
# runtime assertions, fully optimized
assertion: CXXFLAGS += -O3
# no assertion or debug, with debugging info
profile: CXXFLAGS += -O3 -g3 -DNDEBUG

release debug assertion profile &: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

iwyu: clean
	make -k CXX=include-what-you-use

#
# Please note that although we're handling dependencies automatically with -MMD,
# files that includes Flex or Bison-generated files still have to make such
# dependency explicit to enforce the ordering.
#

main.o: %.o: %.cc y.tab.hh

lexer: lexer.l
	$(LEX) -o lex.yy.cc $<

parser: parser.y lex.yy.cc
	$(YACC) $(YFLAGS) $< -o y.tab.cc

clean:
	rm -rf *.o $(TARGET) $(OBJS) $(DEPS)

help:
	@echo "$(TARGET)"
	@echo
	@echo "Target rules:"
	@echo "    lexer      - Generates lexer file (requires Flex)"
	@echo "    parser     - Generates parser file (requires Bison)"
	@echo "    release    - Compiles and generates optimized binary file"
	@echo "    debug      - Compiles and generates binary file with"
	@echo "                 debug messages and less optimizations"
	@echo "    assertion  - Compiles and generates optimized binary file"
	@echo "                 with runtime assertion"
	@echo "    profile    - Compiles and generates optimized binary file"
	@echo "                 with debugging information"
	@echo "    iwyu       - Checks whether all uses are included"
	@echo "    clean      - Cleans the project by removing binaries"
	@echo "    help       - Prints this help message"

-include $(DEPS)
