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
 *    load by calling the exact same FisherYatesShuffle() the live game
 *    used — no need to also snapshot the resulting 81-cell board.
 *
 * 2. We record high-level *intents* (reveal cell / toggle flag at a given
 *    timestamp), not raw mouse coordinates or per-cell flood-fill results.
 *    Flood fill is deterministic given the mine layout, so replaying the
 *    same intent through the same PerformReveal()/PerformToggleFlag()
 *    functions that live play uses (board.h) reproduces the exact same
 *    board state. This "single code path for live input and replay input"
 *    pattern is also the property a future constant-propagation solver will
 *    want: it evaluates hypothetical reveals through the same logic, not a
 *    reimplementation of it.
 *
 * 3. On-disk layout (native struct packing — fine for a single-machine
 *    tool; switch to explicit fixed-width serialization if you ever need
 *    cross-machine portability):
 *
 *      ReplayHeader (internal to replay.c; seed + first click + counts)
 *      ReplayEvent[header.eventCount]
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

// Events recorded during the live game currently in progress
extern ReplayLog currentLog;

// Events loaded from disk, currently being played back
extern ReplayLog activeReplay;

// True while a loaded replay is driving the board instead of live input
extern bool isReplaying;

// Seconds elapsed since the current game (live or replay) state
extern f32 gameClock;

void ReplayLogInit(ReplayLog *log);
void ReplayLogFree(ReplayLog *log);
void ReplayLogPush(ReplayLog *log, EventType type, int row, int col);

// Writes `currentLog` plus the current game's seed / first-click to `path`.
// Fails if no mines have been placed yet (nothing to replay)
bool SaveReplay(const char *path);

// Loads `path`, regenerates its mine layout, and starts driving the board
// from its event log. Board input should be ignored while isReplaying
bool StartReplayPlayback(const char *path);

// Advances playback by `dt` seconds, applying any events whose timestamp
// has been reached. Call once per frame while isReplaying is true
void UpdateReplayPlayback(f32 dt);

#endif // !REPLAY_H
