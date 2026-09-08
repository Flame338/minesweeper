#ifndef TESTS_H
#define TESTS_H

#include <stdbool.h>
#include <stdio.h>

#include "../include/board.h"
#include "../include/replay.h"

/* Pass/fail counters, defined once in test_runner.c. These count individual
 * CHECK(...) assertions (so a loop of 10,000 draws contributes 10,000). The
 * runner reports both the per-test-case result and this assertion total. */
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

/* Each test file keeps its own `static Game g;` fixture (zero-initialized at
 * file scope). A case starts with either:
 *
 *   GameNewWithSeed(&g, seed);  // deterministic, playable game
 *   GameReset(&g);              // blank board to build an arbitrary state on
 *
 * The old clean_board() no longer exists: a fresh Game *is* the reset.
 */

#endif /* !TESTS_H */
