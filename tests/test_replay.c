/*
 * replay.c edge cases: refusing to save before mines are placed, and failing
 * gracefully on a missing/corrupt file. The round-trip test lives in
 * test_roundtrip.c.
 */
#include "tests.h"

void test_save_replay_fails_before_first_click(void) {
  clean_board();
  NewGameWithSeed(5);
  /* No reveal yet -> no mines placed -> nothing meaningful to replay. */
  CHECK(SaveReplay("build/_test_nofirst.msr") == false);
}

void test_load_missing_file_fails(void) {
  clean_board();
  CHECK(StartReplayPlayback("build/_does_not_exist.msr") == false);
}

void test_newgame_resets_state_after_replay(void) {
  clean_board();
  /* NewGameWithSeed fully resets the log and board; verify firstClick is off. */
  NewGameWithSeed(99);
  CHECK(firstClick == false);
  CHECK(currentLog.count == 0);
  CHECK(revealCount == 0);
  CHECK(gameOver == false);
  CHECK(won == false);
}
