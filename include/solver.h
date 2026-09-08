#ifndef SOLVER_H
#define SOLVER_H

#include <stdbool.h>

#include "config.h"
#include "board.h"

/*
 * Constraint-propagation solver for Minesweeper.
 *
 * It reads the revealed numbered cells of a Board — the `grid` passed in by
 * the caller (a Game's grid) — and deduces two kinds of cells:
 *
 *   - a SAFE cell: hidden, unflagged, and guaranteed not to be a mine under
 *     any mine placement consistent with the revealed numbers;
 *   - a MINE cell: hidden, unflagged, and guaranteed to be a mine.
 *
 * Assumptions and scope:
 *   - Flagged cells are treated as correct mines (standard for a hint engine).
 *   - Deductions are conservative: a cell is only marked safe/mine if an
 *     outright proof exists, never as a probabilistic guess. No deduction can
 *     be reversed, so the result never over-claims.
 *   - A board whose flags contradict a revealed number (e.g. a revealed "1"
 *     with two adjacent flags) is INCONSISTENT; the solver reports it and
 *     withholds hints rather than producing confidently wrong ones.
 *
 * One entry point, one pass: SolverAnalyse runs the deduction once and
 * returns consistency plus both deduced sets (see ADR-0003). Callers never
 * re-run the engine per query, and they can't misread partial deductions:
 * when the board is inconsistent the safe/mine lists are empty.
 *
 * All flattened cell indices are row*COLUMNS+col.
 */

// Result of a single full analysis of a board.
typedef struct {
  bool consistent; // false: flags contradict a revealed number; lists empty
  int safeCount;   // number of entries in safe[]
  int safe[ROWS * COLUMNS];
  int mineCount;   // number of entries in mine[]
  int mine[ROWS * COLUMNS];
} SolverResult;

// Runs one full deduction pass over `grid` and writes the result to `out`.
// On an inconsistent board, consistent=false and both lists are empty (the
// partial deductions of a broken board are withheld, never reported).
void SolverAnalyse(const Cell grid[ROWS][COLUMNS], SolverResult *out);

#endif /* !SOLVER_H */
