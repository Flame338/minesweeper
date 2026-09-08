#ifndef REPLAY_H
#define REPLAY_H

#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------------
 * Replay system
 *
 * Design notes:
 *
 * 1. Because mine placement runs on PCG32 instead of libc's rand() (see
 *    pcg32.h), the mine layout is fully reproducible from just two things:
 *    the seed and which cell was clicked first (mine placement excludes
 *    that cell). A replay file stores those and regenerates the layout on
 *    load by calling the exact same GamePlantMines() the live game used —
 *    no need to also snapshot the resulting 81-cell board.
 *
 * 2. We record high-level *intents* (reveal cell / toggle flag at a given
 *    timestamp), not raw mouse coordinates or per-cell flood-fill results.
 *    Flood fill is deterministic given the mine layout, so replaying the
 *    same intent through the same GameReveal()/GameToggleFlag() functions
 *    that live play uses (board.h) reproduces the exact same board state.
 *    This "single code path for live input and replay input" pattern is
 *    also what a constraint-propagation solver wants: it evaluates
 *    hypothetical reveals through the same logic, not a reimplementation.
 *
 * 3. On-disk layout (native struct packing — fine for a single-machine
 *    tool; switch to explicit fixed-width serialization if you ever need
 *    cross-machine portability):
 *
 *      ReplayHeader (internal to replay.c; seed + first click + counts)
 *      ReplayEvent[header.eventCount]
 *
 * A Game owns two logs: `log` (events recorded during live play) and
 * `playback` (events loaded from disk, driving the board while the game's
 * isReplaying flag is set). The functions below operate on whichever log
 * the caller names; the Game-level entry points (GameSaveReplay,
 * GameStartReplayPlayback, GameUpdateReplayPlayback) wire those logs to the
 * live game state. See board.h for the Game type.
 * ------------------------------------------------------------------------ */

typedef float f32;
typedef uint8_t u8;

typedef enum {
  EVT_REVEAL = 0,
  EVT_TOGGLE_FLAG = 1,
} EventType;

typedef struct {
  f32 timeStamp;
  u8 type;
  u8 row;
  u8 col;
} ReplayEvent;

typedef struct {
  ReplayEvent *events;
  int count;
  int capacity;
} ReplayLog;

typedef struct Game Game;

void ReplayLogInit(ReplayLog *log);
void ReplayLogFree(ReplayLog *log);

// Appends an event stamped with `timeStamp` (normally the game's clock).
void ReplayLogPush(ReplayLog *log, f32 timeStamp, EventType type, int row,
                   int col);

// Writes `game->log` plus the game's seed / first-click to `path`.
// Refuses (false) unless the context is a coherent live game — mines placed
// (firstClick), at least one recorded move, and not replaying (GameCanSave;
// see ADR-0003). A replay context is never savable.
bool GameSaveReplay(const Game *game, const char *path);

// Loads `path` into a fresh game state, regenerates its mine layout, and
// starts driving the board from its event log. Board input should be
// ignored while game->isReplaying is true.
bool GameStartReplayPlayback(Game *game, const char *path);

// Advances playback by `dt` seconds, applying any events whose timestamp
// has been reached. Call once per frame while game->isReplaying is true.
void GameUpdateReplayPlayback(Game *game, f32 dt);

#endif // !REPLAY_H
