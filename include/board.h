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

extern Cell grid[ROWS][COLUMNS];
extern bool gameOver;
extern bool won;
extern int BOMBS;
extern int revealCount;

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

#endif // !BOARD_H
