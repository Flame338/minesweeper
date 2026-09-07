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
