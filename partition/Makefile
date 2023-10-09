TARGET = Lab1
CXX = g++
CXXFLAGS = -std=c++17 -g3 -Wall -MMD -Iinclude

OBJS := $(shell find . -name "*.cc")
OBJS := $(OBJS:.cc=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
