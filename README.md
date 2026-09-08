# Minesweeper Clone using C & Raylib

## Features
- Flood-fill reveal
- Constraint-propagation solver
- Hint System
- Replay system that serializes/deserializes game-state to disk

## Building

Developed against **raylib 6.0**. The Makefile is cross-platform: `PLATFORM`
defaults to the detected host OS (`win` or `linux`), and you can override it to
cross-compile.

raylib is vendored (headers in `raylib/include`, a static `libraylib.a` in
`raylib/lib`) and is git-ignored. `make raylib-get` downloads the pinned raylib
6.0 release for the current `PLATFORM` and extracts it into `./raylib`; run it
first if `raylib/` is missing. Headers are identical across platforms, so only
the library file differs.

- **Windows (MinGW-w64 / w64devkit):** `make raylib-get` then `make`
  → `bin/minesweeper.exe` (statically linked, no raylib/GCC runtime DLLs).
- **Linux (WSL / any x86_64 Linux):** `make raylib-get` then `make`
  → `bin/minesweeper`. Needs the X11/GL system libs (e.g. `sudo apt install
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`) to link.
- **Cross-compile Windows from Linux:** `make raylib-get PLATFORM=win` then
  `make PLATFORM=win CC=x86_64-w64-mingw32-gcc`.
- **Run:** `make run`

The Makefile links the platform libraries raylib's desktop backend needs:
Windows uses `-lopengl32 -lgdi32 -lwinmm -luser32 -lshell32`; Linux uses
`-lGL -lm -lpthread -ldl -lrt -lX11`.

A GitHub Actions workflow (`.github/workflows/ci.yml`) runs the test suite on
every change and, on a `v*` tag, builds both the Linux and Windows binaries and
attaches them (plus checksums) to the release.

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

