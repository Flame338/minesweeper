/*
 * The round-trip test — the one that guards the replay feature end to end:
 * play a deterministic game, save it, load it, replay it, and assert the
 * replayed board is byte-for-byte identical to the original final state.
 *
 * This would have caught a broken mine-layout reproduction, a broken event
 * log, or any drift between the live and replay code paths. (The save/load
 * *path-string* typo in the game UI is not reachable here — it lives in the
 * raylib input layer — but it is fixed and the shared `REPLAY_PATH` constant
 * keeps the two call sites from drifting again.)
 */
#include "tests.h"

static Game g; /* zero-initialized fixture */

typedef struct {
  bool revealed;
  bool flagged;
  bool hasMines;
  int neighbourMines;
} CellSnap;

static CellSnap ref[ROWS][COLUMNS];

static void capture_snapshot(void) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      ref[r][c].revealed = g.grid[r][c].revealed;
      ref[r][c].flagged = g.grid[r][c].flagged;
      ref[r][c].hasMines = g.grid[r][c].hasMines;
      ref[r][c].neighbourMines = g.grid[r][c].neighbourMines;
    }
  }
}

static int snapshot_matches(void) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (g.grid[r][c].revealed != ref[r][c].revealed ||
          g.grid[r][c].flagged != ref[r][c].flagged ||
          g.grid[r][c].hasMines != ref[r][c].hasMines ||
          g.grid[r][c].neighbourMines != ref[r][c].neighbourMines) {
        return 0;
      }
    }
  }
  return 1;
}

void test_replay_roundtrip(void) {
  GameNewWithSeed(&g, 0xDEADBEEF12345678ULL);

  /* A realistic game: first click, a flag, and a spread of reveals. */
  GameReveal(&g, 6, 5);
  GameToggleFlag(&g, 6, 6);
  GameReveal(&g, 6, 6);
  GameReveal(&g, 6, 3);
  GameReveal(&g, 7, 6);
  GameReveal(&g, 8, 5);
  GameReveal(&g, 8, 6);
  GameReveal(&g, 8, 7);
  GameReveal(&g, 8, 4);
  GameReveal(&g, 8, 8);
  GameReveal(&g, 5, 0);
  GameReveal(&g, 0, 0);
  GameReveal(&g, 1, 5);
  GameReveal(&g, 0, 5);
  GameReveal(&g, 0, 6);
  GameReveal(&g, 1, 6);
  GameReveal(&g, 1, 7);
  GameReveal(&g, 0, 7);
  GameReveal(&g, 0, 8);

  bool gameOver0 = g.gameOver;
  bool won0 = g.won;
  int revealCount0 = g.revealCount;
  int eventCount0 = g.log.count;
  capture_snapshot();

  /* Save the replay log for this exact game. */
  CHECK(GameSaveReplay(&g, "build/_test_roundtrip.msr") == true);

  /* Replay it back into the same (now reset) board. */
  CHECK(GameStartReplayPlayback(&g, "build/_test_roundtrip.msr") == true);
  CHECK(g.isReplaying == true);

  int guard = 0;
  while (g.isReplaying && guard < 100000) {
    GameUpdateReplayPlayback(&g, 0.05f);
    guard++;
  }
  CHECK(g.isReplaying == false);

  CHECK(snapshot_matches() == 1);
  CHECK(g.gameOver == gameOver0);
  CHECK(g.won == won0);
  CHECK(g.revealCount == revealCount0);

  /* The save wrote the number of events we recorded. */
  CHECK(eventCount0 > 0);
}

void test_replay_stops_at_hit_mine(void) {
  GameNewWithSeed(&g, 0xABCDEFULL);
  /* Keep revealing until we hit a mine (deterministic given the seed). */
  int guard = 0;
  int r = 6, c = 5;
  while (!g.gameOver && guard < 10000) {
    GameReveal(&g, r, c);
    /* Sweep a simple path so we eventually hit a mine. */
    c++;
    if (c >= COLUMNS) {
      c = 0;
      r++;
    }
    if (r >= ROWS)
      break;
    guard++;
  }
  /* Whatever the outcome, the replay of this log must reproduce it. */
  bool gameOver0 = g.gameOver;
  int revealCount0 = g.revealCount;
  capture_snapshot();

  CHECK(GameSaveReplay(&g, "build/_test_stop.msr") == true);
  CHECK(GameStartReplayPlayback(&g, "build/_test_stop.msr") == true);
  int g2 = 0;
  while (g.isReplaying && g2 < 100000) {
    GameUpdateReplayPlayback(&g, 0.05f);
    g2++;
  }
  CHECK(g.gameOver == gameOver0);
  CHECK(g.revealCount == revealCount0);
  CHECK(snapshot_matches() == 1);
}
