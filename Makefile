# Makefile for Minesweeper (raylib) — cross-platform (Windows/MinGW + Linux)
#
# Expected project layout:
#   .
#   ├── Makefile
#   ├── src/        <- your .c / .cpp source files go here
#   ├── include/    <- your own .h headers go here (optional)
#   ├── build/      <- object files land here (auto-created)
#   ├── bin/        <- final executable lands here (auto-created)
#   └── raylib/     <- vendored raylib (git-ignored; `make raylib-get` fetches it)
#
# Building:
#   Windows (MinGW/w64devkit):        make                      -> bin/minesweeper.exe
#   Linux (WSL / CI):                 make raylib-get && make   -> bin/minesweeper
#   Cross-compile Windows on Linux:   make raylib-get PLATFORM=win \
#                                       make PLATFORM=win CC=x86_64-w64-mingw32-gcc
#
# PLATFORM is "win" or "linux". It defaults to the detected host OS; override it
# (make PLATFORM=win) to cross-compile. The raylib variant and the system link
# libraries both follow PLATFORM, so a win build always uses the
# win64_mingw-w64 raylib and the Windows import libs regardless of the build host.

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
SRC_DIR    := src
INC_DIR    := include
BUILD_DIR  := build
BIN_DIR    := bin
RAYLIB_DIR := raylib
TEST_DIR   := tests
TOOLS_DIR  := tools

# --- Platform detection (defaults the raylib variant / target extension) ---
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Linux)
    DETECTED_PLATFORM := linux
else
    DETECTED_PLATFORM := win
endif
PLATFORM ?= $(DETECTED_PLATFORM)

# raylib release used by `make raylib-get` (pinned; matches the vendored header).
RAYLIB_TAG := 6.0
RAYLIB_BASE_URL := https://github.com/raysan5/raylib/releases/download/$(RAYLIB_TAG)

ifeq ($(PLATFORM),win)
    EXE_SUFFIX := .exe
    # System/import libraries raylib's GLFW/Win32 backend needs (OpenGL, GDI,
    # multimedia, user32, shell32). -static-libgcc keeps libgcc_s_seh-1.dll out
    # of the deployed exe so the binary is self-contained.
    # -mwindows makes raylib's WinMain shim the entry point, so the shipped
    # exe is a GUI-subsystem binary (no console window next to the game).
    LDFLAGS := -L$(RAYLIB_DIR)/lib -lraylib -lopengl32 -lgdi32 -lwinmm \
               -luser32 -lshell32 -lm -static-libgcc -mwindows
    RAYLIB_URL := $(RAYLIB_BASE_URL)/raylib-$(RAYLIB_TAG)_win64_mingw-w64.zip
else ifeq ($(PLATFORM),linux)
    EXE_SUFFIX :=
    # System libraries raylib's GLFW/OpenGL desktop backend needs on Linux.
    LDFLAGS := -L$(RAYLIB_DIR)/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    RAYLIB_URL := $(RAYLIB_BASE_URL)/raylib-$(RAYLIB_TAG)_linux_amd64.tar.gz
else
    $(error PLATFORM must be "win" or "linux", got "$(PLATFORM)")
endif

# Output binary name (.exe on Windows; plain name on Linux)
TARGET := $(BIN_DIR)/minesweeper$(EXE_SUFFIX)

# Headless runnables that reuse the raylib-free game logic (board/pcg32/replay)
CORE_SRCS := $(SRC_DIR)/board.c $(SRC_DIR)/pcg32.c $(SRC_DIR)/replay.c \
             $(SRC_DIR)/solver.c
TOOL_EXE  := $(BIN_DIR)/ms_tool$(EXE_SUFFIX)
TEST_EXE  := $(BUILD_DIR)/test_minesweeper$(EXE_SUFFIX)

# Find all source files automatically
SOURCES := $(wildcard $(SRC_DIR)/*.$(SRC_EXT))
OBJECTS := $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(BUILD_DIR)/%.o,$(SOURCES))

# Compiler flags
# -I$(INC_DIR): so #include "yourheader.h" resolves to include/
# -I$(RAYLIB_DIR)/include: raylib public headers, vendored in ./raylib/
# NOTE: the game sources include the vendored raylib.h, so building the game
# needs raylib fetched first (`make raylib-get`). The rule files and CORE_SRCS
# are raylib-free and build without it.
CFLAGS := -Wall -Wextra -std=c11 -I$(INC_DIR) -I$(RAYLIB_DIR)/include

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

# Fetch the platform-appropriate raylib 6.0 release into ./raylib (git-ignored).
# The archive's top-level folder is `raylib/`, so extracting at the repo root
# overlays its include/ and lib/ into ./raylib. Headers are identical across
# platforms; only lib/libraylib.a differs. Overriding PLATFORM makes this fetch
# the other OS's library (used to cross-compile).
raylib-get:
	@echo "Fetching raylib $(RAYLIB_TAG) for $(PLATFORM)..."
	curl -fsSL -o raylib-$(RAYLIB_TAG).$(if $(findstring win,$(PLATFORM)),zip,tar.gz) "$(RAYLIB_URL)"
ifeq ($(PLATFORM),win)
	unzip -oq raylib-$(RAYLIB_TAG).zip
	rm -f raylib-$(RAYLIB_TAG).zip
else
	tar -xzf raylib-$(RAYLIB_TAG).tar.gz
	rm -f raylib-$(RAYLIB_TAG).tar.gz
endif

# Run the game right after building
run: all
	./$(TARGET)

# Build the headless debug tool (no raylib) into bin/ms_tool
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

.PHONY: all run tools test clean rebuild raylib-get
