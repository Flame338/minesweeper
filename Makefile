# Makefile for Minesweeper (raylib) — Windows (MinGW-w64 / w64devkit)
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
RAYLIB_DIR := raylib
TEST_DIR  := tests
TOOLS_DIR := tools

# Output binary name (.exe required on Windows; gcc appends it if omitted, but
# the `run` target needs the real name to launch the built exe)
TARGET := $(BIN_DIR)/minesweeper.exe

# Headless runnables that reuse the raylib-free game logic (board/pcg32/replay)
CORE_SRCS := $(SRC_DIR)/board.c $(SRC_DIR)/pcg32.c $(SRC_DIR)/replay.c \
             $(SRC_DIR)/solver.c
TOOL_EXE  := $(BIN_DIR)/ms_tool.exe
TEST_EXE  := $(BUILD_DIR)/test_minesweeper.exe

# Find all source files automatically
SOURCES := $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJECTS := $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(BUILD_DIR)/%.o,$(SOURCES))

# Compiler flags
# -I$(INC_DIR): so #include "yourheader.h" resolves to include/
# -I$(RAYLIB_DIR)/include: raylib public headers, vendored in ./raylib/
CFLAGS  := -Wall -Wextra -std=c11 -I$(INC_DIR) -I$(RAYLIB_DIR)/include
# Link against the vendored static raylib plus the Windows system libraries
# that raylib's GLFW/Win32 backend requires (OpenGL, GDI, multimedia, user32,
# shell32). -static-libgcc keeps libgcc_s_seh-1.dll out of the deployed exe.
LDFLAGS := -L$(RAYLIB_DIR)/lib -lraylib -lopengl32 -lgdi32 -lwinmm \
           -luser32 -lshell32 -lm -static-libgcc

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

# Build the headless debug tool (no raylib) into bin/ms_tool.exe
tools: $(TOOL_EXE)

$(TOOL_EXE): $(TOOLS_DIR)/ms_tool.c $(CORE_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TOOLS_DIR)/ms_tool.c $(CORE_SRCS) -lm
	@echo "Built $(TOOL_EXE)"

# Compile and run the test suite
test: $(TEST_EXE)
	./$(TEST_EXE)

$(TEST_EXE): $(TEST_DIR)/test_runner.c $(TEST_DIR)/test_pcg32.c \
             $(TEST_DIR)/test_board.c $(TEST_DIR)/test_replay.c \
             $(TEST_DIR)/test_roundtrip.c $(TEST_DIR)/test_solver.c \
             $(CORE_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_runner.c $(TEST_DIR)/test_pcg32.c \
	    $(TEST_DIR)/test_board.c $(TEST_DIR)/test_replay.c \
	    $(TEST_DIR)/test_roundtrip.c $(TEST_DIR)/test_solver.c \
	    $(CORE_SRCS) -lm
	@echo "Built $(TEST_EXE)"

# Remove build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Rebuild everything from scratch
rebuild: clean all

.PHONY: all run tools test clean rebuild
