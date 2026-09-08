#include "../include/board.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "../include/config.h"
#include "../include/replay.h"
#include "../include/solver.h"

Cell grid[ROWS][COLUMNS];
bool gameOver = false;
bool won = false;
int BOMBS = 10;
int revealCount = 0;
bool firstClick = false;
int firstClickRow = -1;
int firstClickCol = -1;

int hintsRemaining = HINT_BUDGET;
bool hasHint = false;
int hintRow = -1;
int hintCol = -1;

u64 currentSeed = 0;
pcg32_random rng;

bool CheckWin(void) { return revealCount == (ROWS * COLUMNS - BOMBS); }

void ComputeNeighbourCounts(void) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      int count = 0;
      for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
          if (dr == 0 && dc == 0)
            continue;
          int nr = r + dr, nc = c + dc;
          if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLUMNS) {
            count += grid[nr][nc].hasMines;
          }
        }
        grid[r][c].neighbourMines = count;
      }
    }
  }
}

void FisherYatesShuffle(int safeRow, int safeCol) {
  int n = ROWS * COLUMNS;
  int safeIndex = safeRow * COLUMNS + safeCol;

  int *idx = (int *)malloc((size_t)(n - 1) * sizeof(int));
  int k = 0;
  for (int i = 0; i < n; i++) {
    if (i != safeIndex)
      idx[k++] = i;
  }

  int remaining = n - 1;
  for (int i = 0; i < remaining; i++) {
    int j = (int)pcg32_boundedrand_r(&rng, (u32)(i + 1));
    int temp = idx[i];
    idx[i] = idx[j];
    idx[j] = temp;
  }

  for (int i = 0; i < BOMBS; i++) {
    int row = idx[i] / COLUMNS;
    int col = idx[i] % COLUMNS;
    grid[row][col].hasMines = true;
  }
  free(idx);

  ComputeNeighbourCounts();
}

void InitGrid(void) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      grid[i][j] = (Cell){0};
    }
  }
}

void RevealAllMines() {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (grid[r][c].hasMines) {
        grid[r][c].revealed = true;
      }
    }
  }
}

void floodFill(int row, int col) {
  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;
  if (grid[row][col].revealed)
    return;
  if (grid[row][col].flagged)
    return;

  if (grid[row][col].hasMines) {
    grid[row][col].revealed = true;
    RevealAllMines();
    gameOver = true;
    return;
  }

  grid[row][col].revealed = true;
  revealCount++;

  // Hit a numbered cell. Stop spreading
  if (grid[row][col].neighbourMines > 0)
    return;

  // Still blank -> keep spreading to all 8 neighbours
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0)
        continue;
      floodFill(row + dr, col + dc);
    }
  }
}

void PerformReveal(int row, int col) {
  if (grid[row][col].flagged)
    return;

  /* The board is about to change, so a stale hint suggestion is no longer
   * guaranteed valid. (A click on a flagged cell bails out above and keeps the
   * hint, since nothing changed.) */
  hasHint = false;
  hintRow = hintCol = -1;

  if (!isReplaying) {
    if (!firstClick) {
      firstClickRow = row;
      firstClickCol = col;
      FisherYatesShuffle(firstClickRow, firstClickCol);
      firstClick = true;
    }
    ReplayLogPush(&currentLog, EVT_REVEAL, row, col);
  }
  floodFill(row, col);
}

void PerformToggleFlag(int row, int col) {
  if (grid[row][col].revealed)
    return;

  /* Board changes => clear any active hint suggestion. */
  hasHint = false;
  hintRow = hintCol = -1;

  grid[row][col].flagged = !grid[row][col].flagged;

  if (!isReplaying) {
    ReplayLogPush(&currentLog, EVT_TOGGLE_FLAG, row, col);
  }
}

/*
 * True if any revealed non-mine cell carries a Neighbour count > 0 - i.e.
 * there is at least one Constraint to reason from. This is the difference
 * between "no information yet" (HINT_NO_INFO) and "info exists but nothing is
 * provable" (HINT_STUCK).
 */
static bool has_revealed_number(void) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (grid[r][c].revealed && !grid[r][c].hasMines &&
          grid[r][c].neighbourMines > 0) {
        return true;
      }
    }
  }
  return false;
}

HintResult RequestHint(int *row, int *col) {
  if (hintsRemaining <= 0)
    return HINT_EXHAUSTED;

  if (!SolverIsConsistent())
    return HINT_INCONSISTENT;

  int r, c;
  if (!SolverHint(&r, &c)) {
    return has_revealed_number() ? HINT_STUCK : HINT_NO_INFO;
  }

  /* Re-requesting the same, still-active hint (nothing changed) costs nothing:
   * the board is unchanged so the suggestion is still exactly right. */
  if (hasHint && hintRow == r && hintCol == c) {
    if (row) {
      *row = r;
      *col = c;
    }
    return HINT_OK;
  }

  hintsRemaining--;
  hasHint = true;
  hintRow = r;
  hintCol = c;
  if (row) {
    *row = r;
    *col = c;
  }
  return HINT_OK;
}

// Resets the board and replay log, then rolls a fresh random seed.
void NewGame(void) {
  // Combining wall-clock & CPU clock so that two games started at the same
  // second still gets different seeds
  NewGameWithSeed(((u64)time(NULL) << 32) ^ (u64)clock());
}

// Same as NewGame() but with an explicit seed. Exposes the seed as a seam so
// tests and the headless tool can build a fully deterministic board (and thus
// a fully deterministic replay).
void NewGameWithSeed(u64 seed) {
  InitGrid();
  gameOver = false;
  won = false;
  revealCount = 0;
  firstClick = false;
  firstClickRow = -1;
  firstClickCol = -1;
  isReplaying = false;
  gameClock = 0.0f;

  /* A fresh game restores the full hint budget and clears any suggestion. */
  hintsRemaining = HINT_BUDGET;
  hasHint = false;
  hintRow = hintCol = -1;

  ReplayLogFree(&currentLog);
  ReplayLogInit(&currentLog);

  currentSeed = seed;
  pcg32_srandom_r(&rng, currentSeed, 1);
}
