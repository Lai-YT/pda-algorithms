TARGET := Lab3
CXX := g++
CC = $(CXX)
CXXFLAGS = -g3 -std=c++17 -Wall -MMD -Iinclude
CFLAGS = $(CXXFLAGS)

# Note that lex.yy.cpp is excluded deliberately, as "lex.yy.cc" is considered a
# header file (it's included by "y.tab.cc").
OBJS := $(shell find . -name "*.cc" ! -name "y.tab.cc" ! -name "lex.yy.cc" ) y.tab.o
OBJS := $(OBJS:.cc=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

#
# Please note that although we're handling dependencies automatically with -MMD,
# files that includes Flex or Bison-generated files still have to make such
# dependency explicit to enforce the ordering.
#

main.o: %.o: %.cc y.tab.hh

clean:
	rm -rf *.o $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
