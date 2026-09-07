/*
 * board.c tests: grid init, mine placement reproducibility, neighbour counts,
 * flood-fill reveal semantics, flag toggling, and the win condition.
 */
#include "tests.h"

void test_init_grid_is_blank(void) {
  clean_board();
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      CHECK(grid[r][c].revealed == false);
      CHECK(grid[r][c].flagged == false);
      CHECK(grid[r][c].hasMines == false);
      CHECK(grid[r][c].neighbourMines == 0);
    }
  }
}

void test_first_click_places_bombs_and_is_safe(void) {
  clean_board();
  NewGameWithSeed(12345);
  PerformReveal(6, 5);

  CHECK(firstClick == true);
  CHECK(firstClickRow == 6);
  CHECK(firstClickCol == 5);
  /* The first-clicked cell is excluded from mine placement. */
  CHECK(grid[6][5].hasMines == false);
  CHECK(gameOver == false);

  int mines = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      if (grid[r][c].hasMines)
        mines++;
  CHECK(mines == BOMBS);

  /* The safe cell itself is revealed. */
  CHECK(revealCount == 1);
}

void test_same_seed_reproduces_layout(void) {
  clean_board();
  NewGameWithSeed(777);
  PerformReveal(6, 5);
  int minesA[ROWS][COLUMNS];
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      minesA[r][c] = grid[r][c].hasMines;

  clean_board();
  NewGameWithSeed(777);
  PerformReveal(6, 5);
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      CHECK(grid[r][c].hasMines == minesA[r][c]);
}

void test_different_seed_differs(void) {
  clean_board();
  NewGameWithSeed(1);
  PerformReveal(6, 5);
  int minesA[ROWS][COLUMNS];
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      minesA[r][c] = grid[r][c].hasMines;

  clean_board();
  NewGameWithSeed(2);
  PerformReveal(6, 5);
  int different = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      if (grid[r][c].hasMines != minesA[r][c])
        different++;
  CHECK(different > 0);
}

void test_neighbour_counts(void) {
  clean_board();
  grid[1][1].hasMines = true;
  grid[1][3].hasMines = true;
  grid[3][2].hasMines = true;
  ComputeNeighbourCounts();

  /* Hand-derived from the three mines above. */
  CHECK(grid[1][2].neighbourMines == 2); /* (1,1)+(1,3) */
  CHECK(grid[2][2].neighbourMines == 3); /* (1,1)+(1,3)+(3,2) */
  CHECK(grid[2][1].neighbourMines == 2); /* (1,1)+(3,2) */
  CHECK(grid[0][8].neighbourMines == 0); /* far from all mines */
  CHECK(grid[1][1].neighbourMines == 0); /* a mine's neighbours hold no mines */
}

void test_floodfill_expands_and_stops_at_numbers(void) {
  clean_board();
  grid[8][8].hasMines = true; /* single mine in the bottom-right corner */
  ComputeNeighbourCounts();

  floodFill(0, 0);

  CHECK(gameOver == false);
  CHECK(grid[0][0].revealed == true);
  /* Every cell except the mine itself is revealed: the flood spreads through
   * blanks, reveals the boundary numbers and stops there, so only the mine
   * cell — reachable only via a number — stays hidden. */
  CHECK(revealCount == 80);
  CHECK(grid[8][8].revealed == false);
  /* (7,8) is a numbered neighbour of the mine: revealed as the boundary, but
   * the flood did not propagate past it (no game-over, mine still hidden). */
  CHECK(grid[7][8].revealed == true);
  CHECK(grid[7][8].neighbourMines == 1);
}

void test_reveal_mine_loses(void) {
  clean_board();
  grid[4][4].hasMines = true;
  ComputeNeighbourCounts();

  floodFill(4, 4);

  CHECK(gameOver == true);
  CHECK(grid[4][4].revealed == true);
  CHECK(revealCount == 0);
  int minesRevealed = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLUMNS; c++)
      if (grid[r][c].hasMines && grid[r][c].revealed)
        minesRevealed++;
  CHECK(minesRevealed == 1);
}

void test_flag_toggle(void) {
  clean_board();
  PerformToggleFlag(2, 3);
  CHECK(grid[2][3].flagged == true);
  PerformToggleFlag(2, 3);
  CHECK(grid[2][3].flagged == false);

  grid[0][0].revealed = true;
  PerformToggleFlag(0, 0);
  CHECK(grid[0][0].flagged == false); /* cannot flag a revealed cell */
}

void test_checkwin(void) {
  clean_board();
  BOMBS = 10;
  CHECK(CheckWin() == false);
  revealCount = ROWS * COLUMNS - BOMBS;
  CHECK(CheckWin() == true);
  revealCount = ROWS * COLUMNS - BOMBS - 1;
  CHECK(CheckWin() == false);
}
