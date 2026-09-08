/*
 * board.c tests: grid init, mine placement reproducibility, neighbour counts,
 * flood-fill reveal semantics, flag toggling, and the win condition.
 */
#include "tests.h"

static Game g; /* zero-initialized fixture; each test starts with a reset */

void test_init_grid_is_blank(void) {
  GameReset(&g);
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      CHECK(g.grid[r][c].revealed == false);
      CHECK(g.grid[r][c].flagged == false);
      CHECK(g.grid[r][c].hasMines == false);
      CHECK(g.grid[r][c].neighbourMines == 0);
    }
  }
}

void test_first_click_places_bombs_and_is_safe(void) {
  GameNewWithSeed(&g, 12345);
  GameReveal(&g, 6, 5);

  CHECK(g.firstClick == true);
  CHECK(g.firstClickRow == 6);
  CHECK(g.firstClickCol == 5);
  /* The first-clicked cell is excluded from mine placement. */
  CHECK(g.grid[6][5].hasMines == false);
  CHECK(g.gameOver == false);

  int mines = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      if (g.grid[r][c].hasMines)
        mines++;
  CHECK(mines == g.bombCount);

  /* The safe cell itself is revealed. */
  CHECK(g.revealCount == 1);
}

void test_same_seed_reproduces_layout(void) {
  GameNewWithSeed(&g, 777);
  GameReveal(&g, 6, 5);
  int minesA[ROWS][COLUMNS];
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      minesA[r][c] = g.grid[r][c].hasMines;

  GameNewWithSeed(&g, 777);
  GameReveal(&g, 6, 5);
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      CHECK(g.grid[r][c].hasMines == minesA[r][c]);
}

void test_different_seed_differs(void) {
  GameNewWithSeed(&g, 1);
  GameReveal(&g, 6, 5);
  int minesA[ROWS][COLUMNS];
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      minesA[r][c] = g.grid[r][c].hasMines;

  GameNewWithSeed(&g, 2);
  GameReveal(&g, 6, 5);
  int different = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      if (g.grid[r][c].hasMines != minesA[r][c])
        different++;
  CHECK(different > 0);
}

void test_neighbour_counts(void) {
  GameReset(&g);
  g.grid[1][1].hasMines = true;
  g.grid[1][3].hasMines = true;
  g.grid[3][2].hasMines = true;
  GameComputeNeighbourCounts(&g);

  /* Hand-derived from the three mines above. */
  CHECK(g.grid[1][2].neighbourMines == 2); /* (1,1)+(1,3) */
  CHECK(g.grid[2][2].neighbourMines == 3); /* (1,1)+(1,3)+(3,2) */
  CHECK(g.grid[2][1].neighbourMines == 2); /* (1,1)+(3,2) */
  CHECK(g.grid[0][8].neighbourMines == 0); /* far from all mines */
  CHECK(g.grid[1][1].neighbourMines == 0); /* a mine's neighbours hold no mines */
}

void test_floodfill_expands_and_stops_at_numbers(void) {
  GameReset(&g);
  g.grid[8][8].hasMines = true; /* single mine in the bottom-right corner */
  GameComputeNeighbourCounts(&g);

  GameFloodFill(&g, 0, 0);

  CHECK(g.gameOver == false);
  CHECK(g.grid[0][0].revealed == true);
  /* Every cell except the mine itself is revealed: the flood spreads through
   * blanks, reveals the boundary numbers and stops there, so only the mine
   * cell — reachable only via a number — stays hidden. */
  CHECK(g.revealCount == 80);
  CHECK(g.grid[8][8].revealed == false);
  /* (7,8) is a numbered neighbour of the mine: revealed as the boundary, but
   * the flood did not propagate past it (no game-over, mine still hidden). */
  CHECK(g.grid[7][8].revealed == true);
  CHECK(g.grid[7][8].neighbourMines == 1);
}

void test_reveal_mine_loses(void) {
  GameReset(&g);
  g.grid[4][4].hasMines = true;
  GameComputeNeighbourCounts(&g);

  GameFloodFill(&g, 4, 4);

  CHECK(g.gameOver == true);
  CHECK(g.grid[4][4].revealed == true);
  CHECK(g.revealCount == 0);
  int minesRevealed = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      if (g.grid[r][c].hasMines && g.grid[r][c].revealed)
        minesRevealed++;
  CHECK(minesRevealed == 1);
}

void test_flag_toggle(void) {
  GameReset(&g);
  GameToggleFlag(&g, 2, 3);
  CHECK(g.grid[2][3].flagged == true);
  GameToggleFlag(&g, 2, 3);
  CHECK(g.grid[2][3].flagged == false);

  g.grid[0][0].revealed = true;
  GameToggleFlag(&g, 0, 0);
  CHECK(g.grid[0][0].flagged == false); /* cannot flag a revealed cell */
}

void test_checkwin(void) {
  GameReset(&g);
  CHECK(GameCheckWin(&g) == false);
  g.revealCount = ROWS * COLUMNS - g.bombCount;
  CHECK(GameCheckWin(&g) == true);
  g.revealCount = ROWS * COLUMNS - g.bombCount - 1;
  CHECK(GameCheckWin(&g) == false);
}

/* Card 2: the Reveal seam owns Win. Revealing the last non-Mine cell through
 * GameReveal (the one path live play, playback and ms_tool share) must set
 * won — no driver re-derives it. */
void test_reveal_seam_sets_won(void) {
  GameReset(&g);
  g.bombCount = 1;
  g.grid[8][8].hasMines = true; /* single mine, bottom-right corner */
  GameComputeNeighbourCounts(&g);

  /* Mines are already placed by hand; GameReveal must not re-roll them. */
  g.firstClick = true;
  g.firstClickRow = 0;
  g.firstClickCol = 0;

  CHECK(g.won == false);
  GameReveal(&g, 0, 0); /* flood fills everything except the mine */

  CHECK(g.gameOver == false);
  CHECK(g.won == true); /* the seam produced the Win, not a driver poll */
  CHECK(g.revealCount == ROWS * COLUMNS - 1);
}

/* Losing through the seam sets gameOver (Lose) and never also sets won. */
void test_reveal_seam_loses_does_not_win(void) {
  GameReset(&g);
  g.grid[4][4].hasMines = true;
  GameComputeNeighbourCounts(&g);
  g.firstClick = true; /* mines already placed; don't re-roll */

  GameReveal(&g, 4, 4);

  CHECK(g.gameOver == true);
  CHECK(g.won == false);
}

/* --- Card 4: legality queries (ADR-0003) -------------------------------- */

/* Reveal/hint are legal exactly during live, unfinished play. */
void test_can_reveal_hint_live_only(void) {
  GameReset(&g); /* fresh, unfinished, not replaying */
  CHECK(GameCanReveal(&g) == true);
  CHECK(GameCanHint(&g) == true);

  g.isReplaying = true;
  CHECK(GameCanReveal(&g) == false); /* playback drives the board itself */
  CHECK(GameCanHint(&g) == false);
  g.isReplaying = false;

  g.gameOver = true;
  CHECK(GameCanReveal(&g) == false);
  CHECK(GameCanHint(&g) == false);
  g.gameOver = false;

  g.won = true;
  CHECK(GameCanReveal(&g) == false); /* won is terminal: no more clicks */
  CHECK(GameCanHint(&g) == false);
}

/* The budget is NOT part of GameCanHint: an exhausted request must still
 * reach GameHint so it can report HINT_EXHAUSTED. */
void test_can_hint_ignores_budget(void) {
  GameReset(&g);
  g.hintsRemaining = 0;
  CHECK(GameCanHint(&g) == true);
  int r, c;
  CHECK(GameHint(&g, &r, &c) == HINT_EXHAUSTED);
}

/* Saving needs a coherent live game: mines placed AND a real log. A fresh
 * game has neither. */
void test_can_save_requires_live_log(void) {
  GameReset(&g);
  CHECK(GameCanSave(&g) == false); /* no first click, no log */

  g.firstClick = true; /* mines "placed", but the log is still empty */
  CHECK(GameCanSave(&g) == false); /* nothing recorded yet */

  g.log.count = 1; /* a move was made (tests may fake the buffer) */
  CHECK(GameCanSave(&g) == true);

  g.isReplaying = true; /* playback context: log is not being recorded */
  CHECK(GameCanSave(&g) == false);
  g.isReplaying = false;

  g.gameOver = true; /* a lost live game keeps its log -> still savable */
  CHECK(GameCanSave(&g) == true);
  g.gameOver = false;

  g.won = true; /* same for a won live game */
  CHECK(GameCanSave(&g) == true);
}
