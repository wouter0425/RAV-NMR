# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -g -Ilib/include -Iutils/include -Ibenchmark/include

# Directories
SRC_DIRS = lib/src utils/src benchmark/src
INC_DIRS = lib/include utils/include benchmark/include
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(addsuffix /*.cpp, $(SRC_DIRS)))

# Object files (preserving directory structure under obj/)
OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/main

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files (preserve source directory structure)
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean up generated files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets
.PHONY: all clean
