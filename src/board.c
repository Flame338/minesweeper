#include "../include/board.h"

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "../include/config.h"
#include "../include/replay.h"
#include "../include/solver.h"

/*
 * The one reset in the codebase. Every way of starting a game — GameNew,
 * GameNewWithSeed, GameReset, GameStartReplayPlayback — funnels through here,
 * so a new Game field can never be forgotten at one reset site.
 *
 * Requires `game` to be zero-initialized (a fresh `Game game = {0}`) or to
 * hold a previously reset Game (logs freed below, then everything rebuilt).
 */
static void reset_game(Game *game) {
  ReplayLogFree(&game->log);
  ReplayLogFree(&game->playback);

  memset(game, 0, sizeof *game);

  game->bombCount = DEFAULT_BOMBS;
  game->firstClickRow = game->firstClickCol = -1;
  game->hintRow = game->hintCol = -1;
  game->hintsRemaining = HINT_BUDGET;

  ReplayLogInit(&game->log);
  ReplayLogInit(&game->playback);

  /* Defined RNG state even before any Seed is rolled. */
  pcg32_srandom_r(&game->rng, 0, 1);
}

bool GameCheckWin(const Game *game) {
  return game->revealCount == (ROWS * COLUMNS - game->bombCount);
}

void GameComputeNeighbourCounts(Game *game) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      int count = 0;
      for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
          if (dr == 0 && dc == 0)
            continue;
          int nr = r + dr, nc = c + dc;
          if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLUMNS) {
            count += game->grid[nr][nc].hasMines;
          }
        }
      }
      game->grid[r][c].neighbourMines = count;
    }
  }
}

void GamePlantMines(Game *game, int safeRow, int safeCol) {
  int n = ROWS * COLUMNS;
  int safeIndex = safeRow * COLUMNS + safeCol;

  /* ROWS/COLUMNS are compile-time, so the shuffle-index scratch has a
   * fixed, tiny size (80 ints) — a stack array, no heap involved. */
  int idx[ROWS * COLUMNS - 1];
  int k = 0;
  for (int i = 0; i < n; i++) {
    if (i != safeIndex)
      idx[k++] = i;
  }

  int remaining = n - 1;
  for (int i = 0; i < remaining; i++) {
    int j = (int)pcg32_boundedrand_r(&game->rng, (u32)(i + 1));
    int temp = idx[i];
    idx[i] = idx[j];
    idx[j] = temp;
  }

  for (int i = 0; i < game->bombCount; i++) {
    int row = idx[i] / COLUMNS;
    int col = idx[i] % COLUMNS;
    game->grid[row][col].hasMines = true;
  }

  GameComputeNeighbourCounts(game);
}

static void reveal_all_mines(Game *game) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (game->grid[r][c].hasMines) {
        game->grid[r][c].revealed = true;
      }
    }
  }
}

void GameFloodFill(Game *game, int row, int col) {
  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;
  if (game->grid[row][col].revealed)
    return;
  if (game->grid[row][col].flagged)
    return;

  if (game->grid[row][col].hasMines) {
    game->grid[row][col].revealed = true;
    reveal_all_mines(game);
    game->gameOver = true;
    return;
  }

  game->grid[row][col].revealed = true;
  game->revealCount++;

  // Hit a numbered cell. Stop spreading
  if (game->grid[row][col].neighbourMines > 0)
    return;

  // Still blank -> keep spreading to all 8 neighbours
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0)
        continue;
      GameFloodFill(game, row + dr, col + dc);
    }
  }
}

void GameReveal(Game *game, int row, int col) {
  if (game->grid[row][col].flagged)
    return;

  /* The board is about to change, so a stale hint suggestion is no longer
   * guaranteed valid. (A click on a flagged cell bails out above and keeps the
   * hint, since nothing changed.) */
  game->hasHint = false;
  game->hintRow = game->hintCol = -1;

  if (!game->isReplaying) {
    if (!game->firstClick) {
      game->firstClickRow = row;
      game->firstClickCol = col;
      GamePlantMines(game, game->firstClickRow, game->firstClickCol);
      game->firstClick = true;
    }
    ReplayLogPush(&game->log, game->clock, EVT_REVEAL, row, col);
  }
  GameFloodFill(game, row, col);
}

void GameToggleFlag(Game *game, int row, int col) {
  if (game->grid[row][col].revealed)
    return;

  /* Board changes => clear any active hint suggestion. */
  game->hasHint = false;
  game->hintRow = game->hintCol = -1;

  game->grid[row][col].flagged = !game->grid[row][col].flagged;

  if (!game->isReplaying) {
    ReplayLogPush(&game->log, game->clock, EVT_TOGGLE_FLAG, row, col);
  }
}


static bool has_revealed_number(const Game *game) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      if (game->grid[r][c].revealed && !game->grid[r][c].hasMines &&
          game->grid[r][c].neighbourMines > 0) {
        return true;
      }
    }
  }
  return false;
}

HintResult GameHint(Game *game, int *row, int *col) {
  if (game->hintsRemaining <= 0)
    return HINT_EXHAUSTED;

  if (!SolverIsConsistent(game->grid))
    return HINT_INCONSISTENT;

  int r, c;
  if (!SolverHint(game->grid, &r, &c)) {
    return has_revealed_number(game) ? HINT_STUCK : HINT_NO_INFO;
  }

  /* Re-requesting the same, still-active hint (nothing changed) costs nothing:
   * the board is unchanged so the suggestion is still exactly right. */
  if (game->hasHint && game->hintRow == r && game->hintCol == c) {
    if (row) {
      *row = r;
      *col = c;
    }
    return HINT_OK;
  }

  game->hintsRemaining--;
  game->hasHint = true;
  game->hintRow = r;
  game->hintCol = c;
  if (row) {
    *row = r;
    *col = c;
  }
  return HINT_OK;
}


void GameNewWithSeed(Game *game, u64 seed) {
  reset_game(game);
  game->seed = seed;
  pcg32_srandom_r(&game->rng, seed, 1);
}

// Rolls a fresh random seed. Combining wall-clock & CPU clock so that two
// games started at the same second still get different seeds.
void GameNew(Game *game) {
  GameNewWithSeed(game, ((u64)time(NULL) << 32) ^ (u64)clock());
}

void GameReset(Game *game) { reset_game(game); }

void GameFree(Game *game) {
  ReplayLogFree(&game->log);
  ReplayLogFree(&game->playback);
}
