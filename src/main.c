#include "raylib.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 450
#define WINDOW_TITLE "Minesweeper"
#define TARGET_FPS 120

#define ROWS 9
#define COLUMNS 9
#define CELL_SIZE 40

typedef struct {
  bool revealed;
  bool flagged;
  bool hasMines;
  int neighbourMines;
} Cell;

#define BTN_WIDTH 140
#define BTN_HEIGHT 40

Rectangle newGameButton = {.x = 10,
                           .y = ROWS * CELL_SIZE + 10,
                           .width = BTN_WIDTH,
                           .height = BTN_HEIGHT};

Cell grid[ROWS][COLUMNS];
bool gameOver = false;
bool won = false;
int BOMBS = 10;
int revealCount = 0;

bool CheckWin(void) { return revealCount == (ROWS * COLUMNS - BOMBS); }

void FisherYatesShuffle(void) {
  int n = ROWS * COLUMNS;

  int *idx = (int *)malloc(n * sizeof(int));
  for (int i = 0; i < n; i++)
    idx[i] = i;

  for (int i = 0; i < n; i++) {
    int j = rand() % (i + 1);
    int temp = idx[i];
    idx[i] = idx[j];
    idx[j] = temp;
  }

  for (int i = 0; i < BOMBS; i++) {
    int row = idx[i] / COLUMNS;
    int col = idx[i] % COLUMNS;
    grid[row][col].hasMines = true;
  }

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLUMNS; c++) {
      int count = 0;
      for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
          if (dr == 0 && dc == 0)
            continue;
          int nr = r + dr, nc = c + dc;
          if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLUMNS)
            count += grid[nr][nc].hasMines;
        }
      grid[r][c].neighbourMines = count;
    }
  }
}

void InitGrid(void) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      grid[i][j] = (Cell){0};
    }
  }
}

void RevealAllMines(void) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0l; c < COLUMNS; c++) {
      if (grid[r][c].hasMines) {
        grid[r][c].revealed = true;
      }
    }
  }
}

void DrawUI(void) {
  DrawRectangleRec(newGameButton, LIGHTGRAY);
  DrawRectangleLinesEx(newGameButton, 2, DARKGRAY);
  DrawText("New Game", 10, ROWS * CELL_SIZE + 60, 20, GREEN);
}

void DrawMinesweeperGrid(void) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      int x = j * CELL_SIZE;
      int y = i * CELL_SIZE;

      Cell cell = grid[i][j];

      Color fill = LIGHTGRAY;
      if (cell.revealed)
        fill = RAYWHITE;
      if (cell.flagged)
        fill = YELLOW;
      DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, fill);
      DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, DARKGRAY);

      if (cell.revealed && !cell.hasMines && cell.neighbourMines > 0) {
        DrawText(TextFormat("%d", cell.neighbourMines), x + CELL_SIZE / 3,
                 y + CELL_SIZE / 4, 20, DARKBLUE);
      }

      // If revealed and is a mine, draw a simple make
      if (cell.revealed && cell.hasMines) {
        DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 3.5,
                   BLACK);
      }
    }
  }

  DrawUI();

  if (won) {
    DrawText("You Win!", 10, ROWS * COLUMNS * 4.5, 20, GREEN);
  } else if (gameOver) {
    DrawText("Game Over", 10, ROWS * COLUMNS * 4.5, 20, RED);
  }
}

void floodFill(int row, int col) {
  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;
  if (grid[row][col].revealed)
    return;
  if (grid[row][col].flagged) {
    return;
  }
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

  // Still blank -> keep spreading to all 8 neighbous
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      if (dr == 0 && dc == 0)
        continue;
      floodFill(row + dr, col + dc);
    }
  }
}

void NewGame(void) {
  InitGrid();
  FisherYatesShuffle();
  gameOver = false;
  won = false;
  revealCount = 0;
}

void HandleInput(void) {
  Vector2 mouse = GetMousePosition();

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      CheckCollisionPointRec(mouse, newGameButton)) {
    NewGame();
    return;
  }

  int col = (int)(mouse.x / CELL_SIZE);
  int row = (int)(mouse.y / CELL_SIZE);

  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (!grid[row][col].flagged) {
      floodFill(row, col);
    }
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    if (!grid[row][col].revealed) {
      grid[row][col].flagged = !grid[row][col].flagged;
    }
  }
}

int main(void) {
  srand(time(NULL));
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(TARGET_FPS);

  NewGame();

  while (!WindowShouldClose()) {

    if (!gameOver) {
      HandleInput();
    }

    if (!gameOver && CheckWin()) {
      won = true;
    }
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawMinesweeperGrid();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
