# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

_No unreleased changes yet._

## [0.1.0] — 2026-09-08

First release. Ships a self-contained Minesweeper for Windows and Linux, built
on a deterministic, seeded mine layout with a replay system and a
constraint-propagation solver.

### Added
- Full Minesweeper gameplay on a seeded 9×9 board built with raylib.
  - Skeleton windowed application (PSE-17).
  - Deterministic mine generation from a seed via the PCG32 RNG (PSE-18,
    PSE-23).
  - Flood-fill reveal that expands the empty region and stops at numbered cells
    (PSE-19).
  - Win/lose end states and game-over flow (PSE-20).
  - New Game option and UI alignment fixes (PSE-21, PSE-22).
- Replay system that records a game's seed, first-click cell, and move intents,
  and replays them to reconstruct the board exactly (PSE-24).
- Constraint-propagation solver plus a guaranteed-safe Hint system with a finite
  per-game Hint budget (PSE-26, PSE-27).
- Headless debug tool (`ms_tool`) that scripts and inspects a game without
  launching a window (PSE-25).
- Cross-platform build & release infrastructure: an OS-aware Makefile, a
  `raylib-get` target that fetches the pinned raylib 6.0 release per platform,
  and a GitHub Actions workflow that runs the test suite on every change and
  publishes standalone binaries to a GitHub Release on a `v*` tag.

### Changed
- Build target is now selectable via `PLATFORM=win|linux`; the raylib variant
  and system link libraries follow the platform, and the game can be
  cross-compiled for Windows from a Linux CI runner.

### Fixed
- Replay load correctly regenerates the mine layout from the stored seed instead
  of relying on a board snapshot.

[0.1.0]: https://github.com/Flame338/minesweeper/releases/tag/v0.1.0
