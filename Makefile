# Makefile for Minesweeper (raylib) — Ubuntu / WSL
#
# Expected project layout:
#   .
#   ├── Makefile
#   ├── src/        <- your .c / .cpp source files go here
#   ├── include/    <- your own .h headers go here (optional)
#   ├── build/      <- object files land here (auto-created)
#   └── bin/        <- final executable lands here (auto-created)

# --- Toggle this if you're writing C++ instead of C ---
CXX_MODE := 0

ifeq ($(CXX_MODE), 1)
    CC      := g++
    SRC_EXT := cpp
else
    CC      := gcc
    SRC_EXT := c
endif

# Directories
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
BIN_DIR   := bin

# Output binary name
TARGET := $(BIN_DIR)/minesweeper

# Find all source files automatically
SOURCES := $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJECTS := $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(BUILD_DIR)/%.o,$(SOURCES))

# Compiler flags
# -I$(INC_DIR): so #include "yourheader.h" resolves to include/
# Uncomment/edit the two lines below only if raylib's headers/lib
# are NOT already on your system's default search paths.
CFLAGS  := -Wall -Wextra -std=c11 -I$(INC_DIR)
# CFLAGS += -I/path/to/raylib/include
LDFLAGS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
# LDFLAGS += -L/path/to/raylib/lib

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Built $(TARGET)"

# Compile each source file into build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create output directories if they don't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Run the game right after building
run: all
	./$(TARGET)

# Remove build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Rebuild everything from scratch
rebuild: clean all

.PHONY: all run clean rebuild
