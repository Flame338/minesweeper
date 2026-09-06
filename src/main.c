#include "raylib.h"
#include <stdbool.h>
#include <stdlib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 450
#define WINDOW_TITLE "Minesweeper"
#define TARGET_FPS 120

#define ROWS 9
#define COLUMNS 9
#define CELL_SIZE 40
#define BOMBS 10

typedef struct {
  bool revealed;
  bool flagged;
  bool hasMines;
  int neighbourMines;
} Cell;

Cell grid[ROWS][COLUMNS];

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

  for (int i = 0; i < n; i++) {
    int row = idx[i] / COLUMNS;
    int col = idx[i] % COLUMNS;
    grid[row * COLUMNS + col]->hasMines = true;
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
            count += grid[nr * COLUMNS + nc]->hasMines;
        }
      grid[r * COLUMNS + c]->neighbourMines = count;
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
        DrawText(TextFormat("%d, cell.neighbourMines"), x + CELL_SIZE / 3,
                 y + CELL_SIZE / 4, 20, DARKBLUE);
      }

      // If revealed and is a mine, draw a simple make
      if (cell.revealed && cell.hasMines) {
        DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 3.5,
                   BLACK);
      }
    }
  }
}

// void testText() {
//   const char dummy_text[] = "Raylib is working on WSL";
//   const int font_size = 20;
//   int textWidth = MeasureText(dummy_text, font_size);
//
//   int textStartX = GetScreenWidth() / 2 - (textWidth / 2);
//   int textStartY = GetScreenHeight() / 2 - (font_size / 2);
//
//   DrawText(dummy_text, textStartX, textStartY, font_size, LIGHTGRAY);
// }

void floodFill(int row, int col) {
  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;
  if (grid[row][col].revealed)
    return;
  if (grid[row][col].flagged)
    return;
  if (grid[row][col].hasMines)
    return;

  grid[row][col].revealed = true;

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

void HandleInput(void) {
  Vector2 mouse = GetMousePosition();
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
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(TARGET_FPS);

  while (!WindowShouldClose()) {
    HandleInput();

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawMinesweeperGrid();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
