/*
 * Constraint-propagation solver tests (solver.h / solver.c).
 *
 * Each test builds a specific board state via the global grid, then asserts the
 * solver's deductions. Expected values are hand-derived from the revealed
 * numbers, not copied from the solver — so a wrong deduction fails the test.
 */
#include "tests.h"
#include "../include/solver.h"

static void set_mine(int r, int c) { grid[r][c].hasMines = true; }
static void set_revealed(int r, int c) { grid[r][c].revealed = true; }
static void set_flag(int r, int c) { grid[r][c].flagged = true; }

/* Rule 1: a revealed "2" whose two mine-neighbours are already flagged =>
 * remaining == 0 => the third neighbour is safe. */
void test_rule1_zero_remaining_safe(void) {
  clean_board();
  set_mine(0, 1);
  set_mine(1, 0);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* assumed mine */
  set_flag(1, 0);     /* assumed mine */

  CHECK(grid[0][0].neighbourMines == 2);
  CHECK(SolverIsConsistent() == true);

  int safe[ROWS * COLUMNS];
  int n = SolverSafeCells(safe);
  CHECK(n == 1);
  CHECK(safe[0] == 1 * COLUMNS + 1); /* (1,1) */

  int row, col;
  CHECK(SolverHint(&row, &col) == true);
  CHECK(row == 1 && col == 1);
}

/* Rule 2: a corner cell whose only three neighbours are all mines =>
 * |unknown| == remaining => all three are mines. */
void test_rule2_unknown_is_all_mines(void) {
  clean_board();
  set_mine(0, 1);
  set_mine(1, 0);
  set_mine(1, 1);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 3 */

  CHECK(grid[0][0].neighbourMines == 3);

  int mines[ROWS * COLUMNS];
  int n = SolverMines(mines);
  CHECK(n == 3);
  int f01 = 0, f10 = 0, f11 = 0;
  for (int i = 0; i < n; i++) {
    if (mines[i] == 1)  f01 = 1;  /* (0,1) */
    if (mines[i] == 9)  f10 = 1;  /* (1,0) */
    if (mines[i] == 10) f11 = 1;  /* (1,1) */
  }
  CHECK(f01 && f10 && f11);

  /* No safe cell is derived from this constraint. */
  int safe[ROWS * COLUMNS];
  CHECK(SolverSafeCells(safe) == 0);
}

/* Subset rule (A ⊆ B, equal remainders => B\A is safe). */
void test_subset_rule_extra_cells_safe(void) {
  clean_board();
  set_mine(1, 0); /* the only mine */
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* (0,0) count == 1, unknowns {(1,0),(1,1)} */
  set_revealed(0, 1); /* (0,1) count == 1, unknowns {(0,2),(1,0),(1,1),(1,2)} */

  CHECK(grid[0][0].neighbourMines == 1);
  CHECK(grid[0][1].neighbourMines == 1);

  int safe[ROWS * COLUMNS];
  int n = SolverSafeCells(safe);
  int f02 = 0, f12 = 0; /* (0,2) idx 2, (1,2) idx 11 */
  for (int i = 0; i < n; i++) {
    CHECK(safe[i] != 9);  /* (1,0) is the mine: never safe */
    CHECK(safe[i] != 10); /* (1,1) ambiguous: never safe */
    if (safe[i] == 2)  f02 = 1;
    if (safe[i] == 11) f12 = 1;
  }
  CHECK(n >= 2);
  CHECK(f02 && f12);

  /* The mine itself is ambiguous between (1,0) and (1,1) -> no definite mine. */
  int mines[ROWS * COLUMNS];
  CHECK(SolverMines(mines) == 0);

  /* The hint returns one of the two guaranteed-safe cells. */
  int row, col;
  CHECK(SolverHint(&row, &col) == true);
  CHECK((row == 0 && col == 2) || (row == 1 && col == 2));
}

/* Hint returns false when there is no revealed-number info (no safe cell). */
void test_hint_false_when_nothing_guaranteed(void) {
  clean_board();
  int row, col;
  CHECK(SolverHint(&row, &col) == false);
  CHECK(SolverSafeCells(NULL) == 0); /* no constraints yet */
}

/* Over-flagged board (a revealed "1" with two flags) is inconsistent; the
 * solver must withhold hints rather than produce wrong ones. */
void test_inconsistent_overflagged(void) {
  clean_board();
  set_mine(0, 1);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 1 */
  set_flag(0, 1);     /* counts as the one mine */
  set_flag(1, 0);     /* extra flag -> remaining == -1 */

  CHECK(grid[0][0].neighbourMines == 1);
  CHECK(SolverIsConsistent() == false);
  int row, col;
  CHECK(SolverHint(&row, &col) == false);
}

/* -------------------------------------------------------------------------
 * Hint-budget / RequestHint tests (board.c).
 *
 * These check the budget-accounting contract: a hint is spent only when a new
 * guaranteed-safe cell is produced; no-info, inconsistent and same-cell
 * re-shows cost nothing; and a new game restores the budget.
 * ------------------------------------------------------------------------- */

void test_hint_budget_starts_full(void) {
  clean_board();
  CHECK(hintsRemaining == HINT_BUDGET);
  CHECK(hasHint == false);
  CHECK(hintRow == -1 && hintCol == -1);
}

void test_hint_no_info_consumes_nothing(void) {
  clean_board();
  int r, c;
  CHECK(RequestHint(&r, &c) == HINT_NO_INFO);
  CHECK(hintsRemaining == HINT_BUDGET);
  CHECK(hasHint == false);
}

/* A provable safe cell is produced; the budget drops by one and the hint cell
 * is recorded. */
void test_hint_success_consumes_one(void) {
  clean_board();
  set_mine(0, 1);
  set_mine(1, 0);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  int r, c;
  CHECK(RequestHint(&r, &c) == HINT_OK);
  CHECK(r == 1 && c == 1);
  CHECK(hintsRemaining == HINT_BUDGET - 1);
  CHECK(hasHint == true);
  CHECK(hintRow == 1 && hintCol == 1);
}

/* Re-requesting the same still-active hint (board unchanged) is free. */
void test_hint_same_cell_reshow_free(void) {
  clean_board();
  set_mine(0, 1);
  set_mine(1, 0);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  int r, c;
  CHECK(RequestHint(&r, &c) == HINT_OK);
  CHECK(hintsRemaining == HINT_BUDGET - 1);

  CHECK(RequestHint(&r, &c) == HINT_OK);
  CHECK(r == 1 && c == 1);
  CHECK(hintsRemaining == HINT_BUDGET - 1); /* unchanged */
  CHECK(hasHint == true);
}

/* An inconsistent board withholds the hint and spends nothing. */
void test_hint_inconsistent_consumes_nothing(void) {
  clean_board();
  set_mine(0, 1);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 1 */
  set_flag(0, 1);     /* the one mine */
  set_flag(1, 0);     /* extra flag > remaining == -1 */
  CHECK(SolverIsConsistent() == false);
  int r, c;
  CHECK(RequestHint(&r, &c) == HINT_INCONSISTENT);
  CHECK(hintsRemaining == HINT_BUDGET);
  CHECK(hasHint == false);
}

/* Zero budget => exhausted, and no new hint is recorded. */
void test_hint_exhausted(void) {
  clean_board();
  hintsRemaining = 0;
  int r, c;
  CHECK(RequestHint(&r, &c) == HINT_EXHAUSTED);
  CHECK(hintsRemaining == 0);
  CHECK(hasHint == false);
}

/* A new game restores the full budget and clears the suggestion. */
void test_hint_budget_reset_on_newgame(void) {
  clean_board();
  set_mine(0, 1);
  set_mine(1, 0);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  CHECK(RequestHint(NULL, NULL) == HINT_OK);
  CHECK(hintsRemaining == HINT_BUDGET - 1);
  CHECK(hasHint == true);

  NewGameWithSeed(42);
  CHECK(hintsRemaining == HINT_BUDGET);
  CHECK(hasHint == false);
  CHECK(hintRow == -1 && hintCol == -1);
}

/* Revealing the hinted cell (a board mutation) clears the suggestion. */
void test_hint_clears_on_mutation(void) {
  clean_board();
  set_mine(0, 1);
  set_mine(1, 0);
  ComputeNeighbourCounts();
  set_revealed(0, 0); /* count == 2 */
  set_flag(0, 1);     /* the two mines, flagged => (1,1) is provably safe */
  set_flag(1, 0);
  int r, c;
  CHECK(RequestHint(&r, &c) == HINT_OK);
  CHECK(hasHint == true);

  firstClick = true; /* mines already placed by hand; don't let reveal re-roll */
  PerformReveal(hintRow, hintCol);
  CHECK(hasHint == false);
}
