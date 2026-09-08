#include "../include/solver.h"

#include <stdbool.h>
#include <stddef.h>

#define MAX_NEIGHBOURS 8
#define MAX_CONSTRAINTS (ROWS * COLUMNS)

/*
 * A constraint is a revealed numbered cell: its Neighbour count must be
 * satisfied entirely by the (unknown + flagged) cells adjacent to it. We keep
 * the raw neighbour list and derive the current "unknown" set each iteration.
 */
typedef struct {
  int cell; /* flattened index of the revealed numbered cell */
  int mines; /* grid[row][col].neighbourMines */
  int n; /* number of in-bounds neighbours */
  int nbr[MAX_NEIGHBOURS]; /* flattened neighbour indices */
} Constraint;

static Constraint constraints[MAX_CONSTRAINTS];
static int constraintCount = 0;

/* Per-cell deduction state, rebuilt from scratch on every propagate() call. */
static bool decidedMine[ROWS * COLUMNS];
static bool decidedSafe[ROWS * COLUMNS];
static bool consistent = true;

/* Build the constraint list from every revealed numbered (non-mine) cell. */
static void collect_constraints(const Cell grid[ROWS][COLUMNS]) {
  constraintCount = 0;
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (!grid[r][c].revealed)
        continue;
      if (grid[r][c].hasMines)
        continue; /* a revealed mine is a lost game, not a deduction source */
      if (grid[r][c].neighbourMines == 0)
        continue; /* no information */

      Constraint *con = &constraints[constraintCount];
      con->cell = r * COLUMNS + c;
      con->mines = grid[r][c].neighbourMines;
      con->n = 0;
      for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
          if (dr == 0 && dc == 0)
            continue;
          int nr = r + dr, nc = c + dc;
          if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLUMNS)
            continue;
          con->nbr[con->n++] = nr * COLUMNS + nc;
        }
      }
      constraintCount++;
    }
  }
}

/*
 * Compute a constraint's current unknown set and remaining mine count.
 * unknown[] receives the hidden/unflagged/undeduced neighbours. remaining is
 * the constraint's mine count minus the neighbours already counted as mines.
 * A leftover negative remaining (after the caller compares it against the
 * unknown set) means the flags contradict this number.
 */
static int unknown_and_remaining(const Constraint *con,
                                 const Cell grid[ROWS][COLUMNS],
                                 int unknown[MAX_NEIGHBOURS],
                                 int *remainingOut) {
  int n = 0;
  int remaining = con->mines;
  for (int k = 0; k < con->n; k++) {
    int idx = con->nbr[k];
    Cell cell = grid[idx / COLUMNS][idx % COLUMNS];
    if (decidedMine[idx]) {
      remaining--; /* flagged, or already deduced as a mine */
    } else if (cell.revealed && cell.hasMines) {
      remaining--; /* a revealed mine (lost state) is a known mine */
    } else if (!cell.revealed && !cell.flagged && !decidedSafe[idx]) {
      unknown[n++] = idx; /* a real, still-unknown candidate */
    }
  }
  *remainingOut = remaining;
  return n;
}

/* True if every element of a[] appears in b[] (a ⊆ b). */
static int is_subset(const int *a, int na, const int *b, int nb) {
  for (int i = 0; i < na; i++) {
    int found = 0;
    for (int j = 0; j < nb; j++) {
      if (b[j] == a[i]) {
        found = 1;
        break;
      }
    }
    if (!found)
      return 0;
  }
  return 1;
}

/* True if idx appears in arr[0..n). */
static int contains(const int *arr, int n, int idx) {
  for (int i = 0; i < n; i++)
    if (arr[i] == idx)
      return 1;
  return 0;
}

/* Run constraint propagation to a fixed point; rebuilds all deduction state. */
static void propagate(const Cell grid[ROWS][COLUMNS]) {
  collect_constraints(grid);

  for (int i = 0; i < ROWS * COLUMNS; i++) {
    decidedMine[i] = false;
    decidedSafe[i] = false;
  }
  /* Flagged cells are assumed to be correct mines. */
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (grid[r][c].flagged)
        decidedMine[r * COLUMNS + c] = true;
    }
  }

  consistent = true;

  bool changed = true;
  while (changed) {
    changed = false;
    for (int a = 0; a < constraintCount; a++) {
      int unA[MAX_NEIGHBOURS];
      int remA;
      int nA = unknown_and_remaining(&constraints[a], grid, unA, &remA);

      if (remA < 0 || remA > nA || (nA == 0 && remA != 0)) {
        consistent = false;
        continue;
      }

      if (remA == 0) {
        /* All unknowns are safe. */
        for (int k = 0; k < nA; k++) {
          if (!decidedSafe[unA[k]]) {
            decidedSafe[unA[k]] = true;
            changed = true;
          }
        }
      } else if (remA == nA) {
        /* All unknowns are mines. */
        for (int k = 0; k < nA; k++) {
          if (!decidedMine[unA[k]]) {
            decidedMine[unA[k]] = true;
            changed = true;
          }
        }
      }

      /* Subset rule against every other constraint (A ⊆ B). */
      for (int b = 0; b < constraintCount; b++) {
        if (b == a)
          continue;
        int unB[MAX_NEIGHBOURS];
        int remB;
        int nB = unknown_and_remaining(&constraints[b], grid, unB, &remB);

        if (remB < 0 || remB > nB || (nB == 0 && remB != 0)) {
          consistent = false;
          continue;
        }
        if (!is_subset(unA, nA, unB, nB))
          continue;

        if (nA == nB && remA != remB) {
          consistent = false; /* same unknown set, conflicting counts */
          continue;
        }
        if (remA == remB) {
          /* B's mines all lie inside A, so B\A are safe. */
          for (int k = 0; k < nB; k++) {
            if (!contains(unA, nA, unB[k]) && !decidedSafe[unB[k]]) {
              decidedSafe[unB[k]] = true;
              changed = true;
            }
          }
        }
        if (remB - remA == nB - nA) {
          /* B\A must hold all of the extra mines. */
          for (int k = 0; k < nB; k++) {
            if (!contains(unA, nA, unB[k]) && !decidedMine[unB[k]]) {
              decidedMine[unB[k]] = true;
              changed = true;
            }
          }
        }
      }
    }
  }
}

void SolverAnalyse(const Cell grid[ROWS][COLUMNS], SolverResult *out) {
  propagate(grid);

  if (!consistent) {
    /* Withhold the partial deductions of a broken board: an inconsistent
     * board's safe/mine sets are not trustworthy, so report none (solver.h). */
    out->consistent = false;
    out->safeCount = 0;
    out->mineCount = 0;
    return;
  }

  out->consistent = true;
  out->safeCount = 0;
  out->mineCount = 0;
  for (int i = 0; i < ROWS * COLUMNS; i++) {
    if (decidedSafe[i]) {
      out->safe[out->safeCount++] = i;
    }
    /* Newly-deduced mines only: already-flagged cells are the assumption,
     * not a deduction (matches the old SolverMines semantics). */
    if (decidedMine[i] && !grid[i / COLUMNS][i % COLUMNS].flagged) {
      out->mine[out->mineCount++] = i;
    }
  }
}
