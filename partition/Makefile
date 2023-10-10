TARGET = Lab1
CXX = g++
CXXFLAGS = -std=c++17 -Wall -MMD -Iinclude

OBJS := $(shell find . -name "*.cc")
OBJS := $(OBJS:.cc=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean release

all: debug

debug: CXXFLAGS += -O0 -g3
release: CXXFLAGS += -O3 -DNDEBUG
debug release &: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
