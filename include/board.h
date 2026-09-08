#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "pcg32.h"

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

extern Cell grid[ROWS][COLUMNS];
extern bool gameOver;
extern bool won;
extern int BOMBS;
extern int revealCount;

// Hints remaining this game. Starts at HINT_BUDGET; reset on a new game and on
// a replay load. Spent only when RequestHint actually produces a new hint.
extern int hintsRemaining;

// The currently highlighted hint cell (the safety suggestion). Cleared whenever
// the board changes so a stale suggestion is never shown. Has no meaning when
// hasHint is false.
extern bool hasHint;
extern int hintRow;
extern int hintCol;

// The board is only randomized once, on the first reveal of a game
// These records when/where that happened so a replay can reproduce it exactly
extern bool firstClick;
extern int firstClickRow;
extern int firstClickCol;

// The seed for the current game's mine layout, and the PCG32 state it
// drives. Exposed so the replay system can save/restore these
extern u64 currentSeed;
extern pcg32_random rng;

void InitGrid(void);
void ComputeNeighbourCounts(void);
void FisherYatesShuffle(int safeRow, int safeCol);
void RevealAllMines(void);
void floodFill(int row, int col);
bool CheckWin(void);

// Starts a brand new game: resets the board, clears the replay log, and
// rolls a fresh seed.
void NewGame(void);

// Like NewGame(), but with an explicit seed. Deterministic boards — the tests
// and the headless tool use this; start replay with NewGameWithSeed if you
// want the same layout twice.
void NewGameWithSeed(u64 seed);

void PerformReveal(int row, int col);
void PerformToggleFlag(int row, int col);

// Requests a guaranteed-safe hint. Spends the budget only when a *new* hint is
// produced (re-showing the still-active hint costs nothing). On HINT_OK writes
// the cell to *row/*col if they are non-NULL. No gating here for replay/game
// over/win — the input layer decides whether the request is legal - so this
// stays a pure, testable board operation.
HintResult RequestHint(int *row, int *col);

#endif // !BOARD_H
