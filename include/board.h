#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "pcg32.h"
#include "replay.h"

typedef struct {
  bool revealed;
  bool flagged;
  bool hasMines;
  int neighbourMines;
} Cell;

/*
 * Outcome of a hint request. Deliberately distinct so the UI can tell the
 * player *why* no hint is given (or why the request was a no-op), rather than
 * collapsing every failure into one message.
 */
typedef enum {
  HINT_OK = 0,          /* a guaranteed-safe cell was produced & shown */
  HINT_EXHAUSTED,       /* the per-game budget is spent */
  HINT_INCONSISTENT,    /* flags contradict a revealed number; hints withheld */
  HINT_STUCK,           /* info exists but no safe cell is provable */
  HINT_NO_INFO,         /* nothing revealed yet to reason from */
} HintResult;

typedef struct Game {
  Cell grid[ROWS][COLUMNS]; 

  bool gameOver;  
  bool won;       
  int bombCount;  
  int revealCount;

  bool firstClick; 
  int firstClickRow;
  int firstClickCol;

  u64 seed;          
  pcg32_random rng;  

  /* Hints remaining this game. Starts at HINT_BUDGET; reset on a new game
   * and on a replay load. Spent only when GameHint actually produces a new
   * hint. */
  int hintsRemaining;

  /* The currently highlighted hint cell (the safety suggestion). Cleared
   * whenever the board changes so a stale suggestion is never shown. Has no
   * meaning when hasHint is false. */
  bool hasHint;
  int hintRow;
  int hintCol;

  ReplayLog log;      
  ReplayLog playback; 
  bool isReplaying;   
  int playbackIndex;  
  f32 clock;          
} Game;

void GameNew(Game *game);
void GameNewWithSeed(Game *game, u64 seed);
void GameReset(Game *game);
void GameFree(Game *game); /* releases the Replay logs */

/* Player moves. These are the one seam live input, replay playback and the
 * tests all cross (see ADR-0001). */
void GameReveal(Game *game, int row, int col);
void GameToggleFlag(Game *game, int row, int col);

// True when every non-Mine Cell has been Revealed.
bool GameCheckWin(const Game *game);

// Requests a guaranteed-safe hint. Spends the budget only when a *new* hint
// is produced (re-showing the still-active hint costs nothing). On HINT_OK
// writes the cell to *row/*col if they are non-NULL. No gating here for
// replay/game over/win — the input layer decides whether the request is
// legal - so this stays a pure, testable board operation.
HintResult GameHint(Game *game, int *row, int *col);

/* Board mechanics. Kept public so the replay loader and the tests can build
 * arbitrary board states; production code reaches them through the moves
 * above. */
void GameComputeNeighbourCounts(Game *game);
void GameFloodFill(Game *game, int row, int col);

// Plants `bombCount` Mines on a fresh Board, excluding (safeRow, safeCol)
// (the First click), then computes Neighbour counts. Fisher-Yates on
// game->rng, so the layout is deterministic given Seed + First click.
void GamePlantMines(Game *game, int safeRow, int safeCol);

#endif // !BOARD_H
