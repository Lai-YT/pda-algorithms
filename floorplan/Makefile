TARGET = Lab2
CXX = g++
CXXFLAGS = -std=c++17 -Wall -MMD -Iinclude

OBJS := $(shell find . -name "*.cc")
OBJS := $(OBJS:.cc=.o)
DEPS = $(OBJS:.o=.d)

.PHONY: all clean release debug assertion profile help iwyu

all: $(TARGET)

$(TARGET): release

# no assertion or debug, fully optimized
release: CXXFLAGS += -O3 -DNDEBUG
# assertion + state printing, few optimized
debug: CXXFLAGS += -O0 -g3 -DDEBUG
# runtime assertions, fully optimized
assertion: CXXFLAGS += -O3
# no assertion or debug, ith debuggin info
profile: CXXFLAGS += -O3 -g3 -DNDEBUG
release debug assertion profile &: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

iwyu: clean
	make -k CXX=include-what-you-use

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

help:
	@echo "$(TARGET)"
	@echo
	@echo "Target rules:"
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
