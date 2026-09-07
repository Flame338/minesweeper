# Minesweeper Clone using C & Raylib

## Features
- Flood-fill reveal
- Constraint-propagation solver
- Hint System
- Replay system that serializes/deserializes game-state to disk

## Building

Developed against **raylib 6.0** on **Windows (MinGW-w64 / w64devkit)**.

- raylib is vendored as a local dependency in `raylib/` (headers in `raylib/include`,
  static `libraylib.a` in `raylib/lib`). It is git-ignored; re-download it if missing:
  `raylib-6.0_win64_mingw-w64.zip` from https://github.com/raysan5/raylib/releases
- Build: `make` (produces `bin/minesweeper.exe`, statically linked)
- Run: `make run`

The Makefile links the Windows system libraries raylib's backend needs
(`-lopengl32 -lgdi32 -lwinmm -luser32 -lshell32`).

## Testing & debug tool

The game rules live in `src/board.c`, `src/pcg32.c`, and `src/replay.c`, which
are raylib-free so they can be tested and scripted headlessly.

- `make test` — builds and runs the test suite (`tests/`, no framework; a tiny
  `CHECK` harness) and exits non-zero if anything fails.
- `make tools` — builds `bin/ms_tool.exe`, a headless REPL driver for the game
  logic. Pipe commands (`new --seed N`, `reveal R C`, `flag R C`, `dump
  --inspect`, `state`, `save PATH`, `load PATH`, `step DT`, `replay PATH`) to
  script and inspect a game without launching the window. The test suite and the
  tool share the same seed-based determinism, so a layout from `ms_tool` can be
  reproduced exactly in a test.

Replay files (`replay.msr`) are runtime artifacts and are git-ignored.

