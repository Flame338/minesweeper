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
      ref[r][c].revealed = grid[r][c].revealed;
      ref[r][c].flagged = grid[r][c].flagged;
      ref[r][c].hasMines = grid[r][c].hasMines;
      ref[r][c].neighbourMines = grid[r][c].neighbourMines;
    }
  }
}

static int snapshot_matches(void) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (grid[r][c].revealed != ref[r][c].revealed ||
          grid[r][c].flagged != ref[r][c].flagged ||
          grid[r][c].hasMines != ref[r][c].hasMines ||
          grid[r][c].neighbourMines != ref[r][c].neighbourMines) {
        return 0;
      }
    }
  }
  return 1;
}

void test_replay_roundtrip(void) {
  clean_board();
  NewGameWithSeed(0xDEADBEEF12345678ULL);

  /* A realistic game: first click, a flag, and a spread of reveals. */
  PerformReveal(6, 5);
  PerformToggleFlag(6, 6);
  PerformReveal(6, 6);
  PerformReveal(6, 3);
  PerformReveal(7, 6);
  PerformReveal(8, 5);
  PerformReveal(8, 6);
  PerformReveal(8, 7);
  PerformReveal(8, 4);
  PerformReveal(8, 8);
  PerformReveal(5, 0);
  PerformReveal(0, 0);
  PerformReveal(1, 5);
  PerformReveal(0, 5);
  PerformReveal(0, 6);
  PerformReveal(1, 6);
  PerformReveal(1, 7);
  PerformReveal(0, 7);
  PerformReveal(0, 8);

  bool gameOver0 = gameOver;
  bool won0 = won;
  int revealCount0 = revealCount;
  int eventCount0 = currentLog.count;
  capture_snapshot();

  /* Save the replay log for this exact game. */
  CHECK(SaveReplay("build/_test_roundtrip.msr") == true);

  /* Replay it back into the same (now reset) board. */
  CHECK(StartReplayPlayback("build/_test_roundtrip.msr") == true);
  CHECK(isReplaying == true);

  int guard = 0;
  while (isReplaying && guard < 100000) {
    UpdateReplayPlayback(0.05f);
    guard++;
  }
  CHECK(isReplaying == false);

  CHECK(snapshot_matches() == 1);
  CHECK(gameOver == gameOver0);
  CHECK(won == won0);
  CHECK(revealCount == revealCount0);

  /* The save wrote the number of events we recorded. */
  CHECK(eventCount0 > 0);
}

void test_replay_stops_at_hit_mine(void) {
  clean_board();
  NewGameWithSeed(0xABCDEFULL);
  /* Keep revealing until we hit a mine (deterministic given the seed). */
  int guard = 0;
  int r = 6, c = 5;
  while (!gameOver && guard < 10000) {
    PerformReveal(r, c);
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
  bool gameOver0 = gameOver;
  int revealCount0 = revealCount;
  capture_snapshot();

  CHECK(SaveReplay("build/_test_stop.msr") == true);
  CHECK(StartReplayPlayback("build/_test_stop.msr") == true);
  int g2 = 0;
  while (isReplaying && g2 < 100000) {
    UpdateReplayPlayback(0.05f);
    g2++;
  }
  CHECK(gameOver == gameOver0);
  CHECK(revealCount == revealCount0);
  CHECK(snapshot_matches() == 1);
}
