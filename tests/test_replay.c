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

/* Card 4 (ADR-0003): a replay context is never savable — the seam refuses
 * during playback AND after it ends (the live log was never re-recorded, so
 * saving would write a meaningless empty .msr). */
void test_save_refused_during_and_after_playback(void) {
  /* Build a real replay file from a short live game. */
  GameNewWithSeed(&g, 4242);
  GameReveal(&g, 6, 5);
  GameReveal(&g, 6, 6);
  GameToggleFlag(&g, 5, 5);
  CHECK(GameSaveReplay(&g, "build/_test_playback_save.msr") == true);

  /* Load it: playback owns the board, the live log is empty. */
  CHECK(GameStartReplayPlayback(&g, "build/_test_playback_save.msr") == true);
  CHECK(g.isReplaying == true);
  CHECK(GameCanSave(&g) == false);
  CHECK(GameSaveReplay(&g, "build/_test_should_not_exist.msr") == false);

  /* Run playback to the end; the live log is still empty, so the context is
   * still not a saveable live game. */
  int guard = 0;
  while (g.isReplaying && guard < 100000) {
    GameUpdateReplayPlayback(&g, 0.05f);
    guard++;
  }
  CHECK(g.isReplaying == false);
  CHECK(g.log.count == 0);
  CHECK(GameCanSave(&g) == false);
  CHECK(GameSaveReplay(&g, "build/_test_should_not_exist.msr") == false);
}
