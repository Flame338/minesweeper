#include "raylib.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_TITLE "Minesweeper"
#define TARGET_FPS 120

#define ROWS 9
#define COLUMNS 9
#define CELL_SIZE 40

#define PANEL_HEIGHT 90
#define WINDOW_WIDTH (COLUMNS * CELL_SIZE)
#define WINDOW_HEIGHT (ROWS * CELL_SIZE + PANEL_HEIGHT)

typedef struct {
  bool revealed;
  bool flagged;
  bool hasMines;
  int neighbourMines;
} Cell;

Color numberColors[9] = {BLANK,  BLUE,       DARKGREEN, RED, DARKBLUE,
                         MAROON, DARKPURPLE, BLACK,     GRAY};

#define BTN_WIDTH 140
#define BTN_HEIGHT 40

Rectangle newGameButton = {.x = (WINDOW_WIDTH - BTN_WIDTH) / 2.0f,
                           .y = ROWS * CELL_SIZE + 15,
                           .width = BTN_WIDTH,
                           .height = BTN_HEIGHT};

Cell grid[ROWS][COLUMNS];
bool gameOver = false;
bool won = false;
int BOMBS = 10;
int revealCount = 0;
bool firstClick = false;

bool CheckWin(void) { return revealCount == (ROWS * COLUMNS - BOMBS); }

void FisherYatesShuffle(int safeRow, int safeCol) {
  int n = ROWS * COLUMNS;
  int safeIndex = safeRow * COLUMNS + safeCol;

  int *idx = (int *)malloc((n - 1) * sizeof(int));
  int k = 0;
  for (int i = 0; i < n; i++) {
    if (i != safeIndex)
      idx[i] = i;
  }
  int remaining = n - 1;
  for (int i = 0; i < remaining; i++) {
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
  free(idx);

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
  int panelY = ROWS * CELL_SIZE;

  DrawRectangle(0, panelY, WINDOW_WIDTH, PANEL_HEIGHT,
                (Color){40, 44, 52, 255});

  Vector2 mouse = GetMousePosition();
  bool hovered = CheckCollisionPointRec(mouse, newGameButton);
  Color btnColor =
      hovered ? (Color){90, 200, 250, 255} : (Color){70, 130, 180, 255};

  DrawRectangleRounded(newGameButton, 0.3f, 8, btnColor);
  const char *label = "New Game";
  int labelWidth = MeasureText(label, 18);
  DrawText(label, newGameButton.x + (newGameButton.width - labelWidth) / 2,
           newGameButton.y + (newGameButton.height - 18) / 2, 18, RAYWHITE);

  const char *status = NULL;
  Color statusColor = RAYWHITE;
  if (won) {
    status = "You Win!";
    statusColor = GREEN;
  } else if (gameOver) {
    status = "Game Over!";
    statusColor = RED;
  }

  if (status != NULL) {
    int statusWidth = MeasureText(status, 20);
    DrawText(status, (WINDOW_WIDTH - statusWidth) / 2,
             newGameButton.y + newGameButton.height + 12, 20, statusColor);
  }
}

void DrawMinesweeperGrid(void) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      int x = j * CELL_SIZE;
      int y = i * CELL_SIZE;

      Cell cell = grid[i][j];

      Color fill;
      if (cell.flagged) {
        fill = GOLD;
      } else if (cell.revealed) {
        fill = ((i + j) % 2 == 0) ? RAYWHITE : (Color){235, 235, 235, 255};
      } else {
        fill = ((i + j) % 2 == 0) ? RAYWHITE : (Color){190, 190, 190, 255};
      }

      DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, fill);
      DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, DARKGRAY);

      if (cell.revealed && !cell.hasMines && cell.neighbourMines > 0) {
        const char *num = TextFormat("%d", cell.neighbourMines);
        int textWidth = MeasureText(num, 20);
        DrawText(num, x + (CELL_SIZE - textWidth) / 2, y + CELL_SIZE / 4, 20,
                 numberColors[cell.neighbourMines]);
      }

      // If revealed and is a mine, draw a simple make
      if (cell.revealed && cell.hasMines) {
        DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 3.5,
                   BLACK);
      }
    }
  }

  DrawUI();
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
  gameOver = false;
  won = false;
  revealCount = 0;
  firstClick = false;
}

void HandleInput(void) {
  Vector2 mouse = GetMousePosition();

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      CheckCollisionPointRec(mouse, newGameButton)) {
    NewGame();
    return;
  }

  if (gameOver)
    return;

  int col = (int)(mouse.x / CELL_SIZE);
  int row = (int)(mouse.y / CELL_SIZE);

  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (!grid[row][col].flagged) {
      if (!firstClick) {
        FisherYatesShuffle(row, col);
        firstClick = true;
      }
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

    HandleInput();

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
