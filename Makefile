# Compiler and flags
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -O3 -march=native -flto -DNDEBUG

# Source files and output binary
SRCS := $(wildcard src/*.cpp)
BIN := nebula

# Default target: compile everything in one step
all:
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(BIN)
	rm -f *.o

# Clean rule
clean:
	rm -f $(BIN)

.PHONY: all clean