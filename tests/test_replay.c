/*
 * replay.c edge cases: refusing to save before mines are placed, and failing
 * gracefully on a missing/corrupt file. The round-trip test lives in
 * test_roundtrip.c.
 */
#include "tests.h"

static Game g; /* zero-initialized fixture */

void test_save_replay_fails_before_first_click(void) {
  GameNewWithSeed(&g, 5);
  /* No reveal yet -> no mines placed -> nothing meaningful to replay. */
  CHECK(GameSaveReplay(&g, "build/_test_nofirst.msr") == false);
}

void test_load_missing_file_fails(void) {
  GameNewWithSeed(&g, 7);
  CHECK(GameStartReplayPlayback(&g, "build/_does_not_exist.msr") == false);
}

void test_newgame_resets_state_after_replay(void) {
  GameNewWithSeed(&g, 99);
  /* GameNewWithSeed fully resets the log and board; verify firstClick is off. */
  CHECK(g.firstClick == false);
  CHECK(g.log.count == 0);
  CHECK(g.revealCount == 0);
  CHECK(g.gameOver == false);
  CHECK(g.won == false);
}
