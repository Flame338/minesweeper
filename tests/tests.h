#ifndef TESTS_H
#define TESTS_H

#include <stdbool.h>
#include <stdio.h>

#include "../include/board.h"
#include "../include/replay.h"

/* Pass/fail counters, defined once in test_runner.c, incremented by CHECK. */
extern int g_tests_pass;
extern int g_tests_fail;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (cond) {                                                                \
      ++g_tests_pass;                                                          \
    } else {                                                                   \
      ++g_tests_fail;                                                          \
      printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);                 \
    }                                                                          \
  } while (0)

/* Reset every piece of global game state to a known deterministic baseline.
 * Leaves the RNG seed alone; call NewGameWithSeed(seed) afterwards to get a
 * fully reproducible board. */
static inline void clean_board(void) {
  InitGrid();
  gameOver = false;
  won = false;
  revealCount = 0;
  firstClick = false;
  firstClickRow = -1;
  firstClickCol = -1;
  isReplaying = false;
  BOMBS = 10;
  gameClock = 0.0f;
  ReplayLogFree(&currentLog);
  ReplayLogInit(&currentLog);
  ReplayLogFree(&activeReplay);
  ReplayLogInit(&activeReplay);
}

#endif /* !TESTS_H */
