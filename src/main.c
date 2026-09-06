#include "raylib.h"

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

Cell grid[ROWS][COLUMNS];

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

void HandleInput(void) {
  Vector2 mouse = GetMousePosition();
  int col = (int)(mouse.x / CELL_SIZE);
  int row = (int)(mouse.y / CELL_SIZE);

  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (!grid[row][col].flagged) {
      grid[row][col].revealed = true;
    }
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    if (!grid[row][col].revealed) {
      grid[row][col].flagged = !grid[row][col].flagged;
    }
  }
}

void testText() {
  const char dummy_text[] = "Raylib is working on WSL";
  const int font_size = 20;
  int textWidth = MeasureText(dummy_text, font_size);

  int textStartX = GetScreenWidth() / 2 - (textWidth / 2);
  int textStartY = GetScreenHeight() / 2 - (font_size / 2);

  DrawText(dummy_text, textStartX, textStartY, font_size, LIGHTGRAY);
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
