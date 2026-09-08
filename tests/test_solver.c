/*
 * Constraint-propagation solver tests (solver.h / solver.c).
 *
 * Each test builds a specific board state on the fixture Game, then asserts
 * the solver's deductions. Expected values are hand-derived from the revealed
 * numbers, not copied from the solver — so a wrong deduction fails the test.
 *
 * All deduction queries go through the one SolverAnalyse seam (ADR-0003):
 * a single pass answers consistency, safe cells and mines together.
 */
#include "tests.h"
#include "../include/solver.h"

static Game g; /* zero-initialized fixture */

static void set_mine(int r, int c) { g.grid[r][c].hasMines = true; }
static void set_revealed(int r, int c) { g.grid[r][c].revealed = true; }
static void set_flag(int r, int c) { g.grid[r][c].flagged = true; }

/* Run one full analysis of the fixture board. */
static SolverResult analyse(void) {
  SolverResult r;
  SolverAnalyse(g.grid, &r);
  return r;
}

/* Rule 1: a revealed "2" whose two mine-neighbours are already flagged =>
 * remaining == 0 => the third neighbour is safe. */
void test_rule1_zero_remaining_safe(void) {
  GameReset(&g);
  set_mine(0, 1);
  set_mine(1, 0);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* assumed mine */
  set_flag(1, 0);     /* assumed mine */

  CHECK(g.grid[0][0].neighbourMines == 2);
  SolverResult r = analyse();
  CHECK(r.consistent == true);
  CHECK(r.safeCount == 1);
  CHECK(r.safe[0] == 1 * COLUMNS + 1); /* (1,1) */
  CHECK(r.mineCount == 0); /* the two neighbours are flagged, not deduced */
}

/* Rule 2: a corner cell whose only three neighbours are all mines =>
 * |unknown| == remaining => all three are mines. */
void test_rule2_unknown_is_all_mines(void) {
  GameReset(&g);
  set_mine(0, 1);
  set_mine(1, 0);
  set_mine(1, 1);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 3 */

  CHECK(g.grid[0][0].neighbourMines == 3);

  SolverResult r = analyse();
  CHECK(r.consistent == true);
  CHECK(r.mineCount == 3);
  int f01 = 0, f10 = 0, f11 = 0;
  for (int i = 0; i < r.mineCount; i++) {
    if (r.mine[i] == 1)  f01 = 1;  /* (0,1) */
    if (r.mine[i] == 9)  f10 = 1;  /* (1,0) */
    if (r.mine[i] == 10) f11 = 1;  /* (1,1) */
  }
  CHECK(f01 && f10 && f11);

  /* No safe cell is derived from this constraint. */
  CHECK(r.safeCount == 0);
}

/* Subset rule (A ⊆ B, equal remainders => B\A is safe). */
void test_subset_rule_extra_cells_safe(void) {
  GameReset(&g);
  set_mine(1, 0); /* the only mine */
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* (0,0) count == 1, unknowns {(1,0),(1,1)} */
  set_revealed(0, 1); /* (0,1) count == 1, unknowns {(0,2),(1,0),(1,1),(1,2)} */

  CHECK(g.grid[0][0].neighbourMines == 1);
  CHECK(g.grid[0][1].neighbourMines == 1);

  SolverResult r = analyse();
  CHECK(r.consistent == true);
  int f02 = 0, f12 = 0; /* (0,2) idx 2, (1,2) idx 11 */
  for (int i = 0; i < r.safeCount; i++) {
    CHECK(r.safe[i] != 9);  /* (1,0) is the mine: never safe */
    CHECK(r.safe[i] != 10); /* (1,1) ambiguous: never safe */
    if (r.safe[i] == 2)  f02 = 1;
    if (r.safe[i] == 11) f12 = 1;
  }
  CHECK(r.safeCount >= 2);
  CHECK(f02 && f12);

  /* The mine itself is ambiguous between (1,0) and (1,1) -> no definite
   * mine is deducible. */
  CHECK(r.mineCount == 0);

  /* The first safe cell is a legal hint target. */
  CHECK(r.safe[0] == 2 || r.safe[0] == 11);
}

/* A board with no revealed-number info has no safe cell (a hint is
 * impossible) yet is trivially consistent. */
void test_hint_false_when_nothing_guaranteed(void) {
  GameReset(&g);
  SolverResult r = analyse();
  CHECK(r.consistent == true);
  CHECK(r.safeCount == 0); /* no constraints yet */
}

/* Over-flagged board (a revealed "1" with two flags) is inconsistent; the
 * solver must report it with empty lists rather than partial deductions. */
void test_inconsistent_overflagged(void) {
  GameReset(&g);
  set_mine(0, 1);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 1 */
  set_flag(0, 1);     /* counts as the one mine */
  set_flag(1, 0);     /* extra flag -> remaining == -1 */

  CHECK(g.grid[0][0].neighbourMines == 1);
  SolverResult r = analyse();
  CHECK(r.consistent == false);
  /* An inconsistent board withholds its partial deductions entirely: no
   * caller can misread a half-trusted safe/mine list (ADR-0003). */
  CHECK(r.safeCount == 0);
  CHECK(r.mineCount == 0);
}

/* -------------------------------------------------------------------------
 * Hint-budget / GameHint tests (board.c).
 *
 * These check the budget-accounting contract: a hint is spent only when a new
 * guaranteed-safe cell is produced; no-info, inconsistent and same-cell
 * re-shows cost nothing; and a new game restores the budget.
 * ------------------------------------------------------------------------- */

void test_hint_budget_starts_full(void) {
  GameReset(&g);
  CHECK(g.hintsRemaining == HINT_BUDGET);
  CHECK(g.hasHint == false);
  CHECK(g.hintRow == -1 && g.hintCol == -1);
}

void test_hint_no_info_consumes_nothing(void) {
  GameReset(&g);
  int r, c;
  CHECK(GameHint(&g, &r, &c) == HINT_NO_INFO);
  CHECK(g.hintsRemaining == HINT_BUDGET);
  CHECK(g.hasHint == false);
}

/* A provable safe cell is produced; the budget drops by one and the hint cell
 * is recorded. */
void test_hint_success_consumes_one(void) {
  GameReset(&g);
  set_mine(0, 1);
  set_mine(1, 0);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  int r, c;
  CHECK(GameHint(&g, &r, &c) == HINT_OK);
  CHECK(r == 1 && c == 1);
  CHECK(g.hintsRemaining == HINT_BUDGET - 1);
  CHECK(g.hasHint == true);
  CHECK(g.hintRow == 1 && g.hintCol == 1);
}

/* Re-requesting the same still-active hint (board unchanged) is free. */
void test_hint_same_cell_reshow_free(void) {
  GameReset(&g);
  set_mine(0, 1);
  set_mine(1, 0);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  int r, c;
  CHECK(GameHint(&g, &r, &c) == HINT_OK);
  CHECK(g.hintsRemaining == HINT_BUDGET - 1);

  CHECK(GameHint(&g, &r, &c) == HINT_OK);
  CHECK(r == 1 && c == 1);
  CHECK(g.hintsRemaining == HINT_BUDGET - 1); /* unchanged */
  CHECK(g.hasHint == true);
}

/* An inconsistent board withholds the hint and spends nothing. */
void test_hint_inconsistent_consumes_nothing(void) {
  GameReset(&g);
  set_mine(0, 1);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 1 */
  set_flag(0, 1);     /* the one mine */
  set_flag(1, 0);     /* extra flag > remaining == -1 */
  SolverResult r = analyse();
  CHECK(r.consistent == false);
  int row, col;
  CHECK(GameHint(&g, &row, &col) == HINT_INCONSISTENT);
  CHECK(g.hintsRemaining == HINT_BUDGET);
  CHECK(g.hasHint == false);
}

/* Zero budget => exhausted, and no new hint is recorded. */
void test_hint_exhausted(void) {
  GameReset(&g);
  g.hintsRemaining = 0;
  int r, c;
  CHECK(GameHint(&g, &r, &c) == HINT_EXHAUSTED);
  CHECK(g.hintsRemaining == 0);
  CHECK(g.hasHint == false);
}

/* A new game restores the full budget and clears the suggestion. */
void test_hint_budget_reset_on_newgame(void) {
  GameReset(&g);
  set_mine(0, 1);
  set_mine(1, 0);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  CHECK(GameHint(&g, NULL, NULL) == HINT_OK);
  CHECK(g.hintsRemaining == HINT_BUDGET - 1);
  CHECK(g.hasHint == true);

  GameNewWithSeed(&g, 42);
  CHECK(g.hintsRemaining == HINT_BUDGET);
  CHECK(g.hasHint == false);
  CHECK(g.hintRow == -1 && g.hintCol == -1);
}

/* Revealing the hinted cell (a board mutation) clears the suggestion. */
void test_hint_clears_on_mutation(void) {
  GameReset(&g);
  set_mine(0, 1);
  set_mine(1, 0);
  GameComputeNeighbourCounts(&g);
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  int r, c;
  CHECK(GameHint(&g, &r, &c) == HINT_OK);
  CHECK(g.hasHint == true);

  g.firstClick = true; /* mines already placed by hand; don't let reveal re-roll */
  GameReveal(&g, g.hintRow, g.hintCol);
  CHECK(g.hasHint == false);
}
