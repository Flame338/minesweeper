#include "../include/board.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "../include/config.h"
#include "../include/replay.h"

Cell grid[ROWS][COLUMNS];
bool gameOver = false;
bool won = false;
int BOMBS = 10;
int revealCount = 0;
bool firstClick = false;
int firstClickRow = -1;
int firstClickCol = -1;

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

void InitGrid() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      if (grid[i][j].hasMines) {
        grid[i][j].revealed = true;
      }
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

  grid[row][col].flagged = !grid[row][col].flagged;

  if (!isReplaying) {
    ReplayLogPush(&currentLog, EVT_TOGGLE_FLAG, row, col);
  }
}

void NewGame(void) {
  InitGrid();
  gameOver = false;
  won = false;
  revealCount = 0;
  firstClick = false;
  firstClickRow = -1;
  firstClickCol = -1;
  isReplaying = false;
  gameClock = 0.0f;

  ReplayLogFree(&currentLog);
  ReplayLogInit(&currentLog);

  // Combining wall-clock & CPU clock so that two games started at the same
  // second still gets different seeds
  currentSeed = ((u64)time(NULL) << 32) ^ (u64)clock();
  pcg32_srandom_r(&rng, currentSeed, 1);
}
