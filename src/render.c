#include "../include/render.h"
#include "../include/board.h"
#include "../include/config.h"
#include "../include/replay.h"

#include <raylib.h>
#include <stddef.h>

static Color numberColors[9] = {BLANK,  BLUE,       DARKGREEN, RED, DARKBLUE,
                                MAROON, DARKPURPLE, BLANK,     GRAY};
Rectangle newGameButton = {.x = (WINDOW_WIDTH - BTN_WIDTH) / 2.0f,
                           .y = ROWS * CELL_SIZE + 15,
                           .width = BTN_WIDTH,
                           .height = BTN_HEIGHT};

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
  } else if (isReplaying) {
    status = "Replaying... (S: save, L: load)";
    statusColor = SKYBLUE;
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

      if (cell.revealed && cell.hasMines) {
        DrawCircle(x + CELL_SIZE / 2, y + CELL_SIZE / 2, CELL_SIZE / 3.5,
                   BLACK);
      }
    }
  }
  DrawUI();
}
