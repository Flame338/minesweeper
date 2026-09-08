#ifndef SOLVER_H
#define SOLVER_H

#include <stdbool.h>

#include "config.h"
#include "board.h"

/*
 * Constraint-propagation solver for Minesweeper.
 *
 * It reads the revealed numbered cells already on the board (the global `grid`)
 * and deduces two kinds of cells:
 *
 *   - a SAFE cell: hidden, unflagged, and guaranteed not to be a mine under any
 *     mine placement consistent with the revealed numbers;
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
 * All flattened cell indices are row*COLUMNS+col.
 */

// False if any revealed number contradicts the current flags (a hint would be
// unreliable). Otherwise true.
bool SolverIsConsistent(void);

// Writes flattened indices of cells guaranteed NOT to be mines into out[].
// Returns the count. out is never modified if it is NULL (just count the safe
// cells that exist).
int SolverSafeCells(int out[ROWS * COLUMNS]);

// Writes flattened indices of cells guaranteed to be mines into out[].
// Returns the count. Only newly-deduced, unflagged cells are reported (already
// flagged cells are treated as the assumption, not a deduction).
int SolverMines(int out[ROWS * COLUMNS]);

// If a guaranteed-safe cell is deducible, writes its coordinates to *row/*col
// and returns true. Returns false if the board is inconsistent or no safe cell
// is provable (a guess would be required).
bool SolverHint(int *row, int *col);

#endif /* !SOLVER_H */
