#include "../include/render.h"
#include "../include/board.h"
#include "../include/config.h"
#include "../include/replay.h"

#include <raylib.h>
#include <math.h>
#include <stddef.h>

static Color numberColors[9] = {BLANK,  BLUE,       DARKGREEN, RED, DARKBLUE,
                                MAROON, DARKPURPLE, BLANK,     GRAY};

/* Two side-by-side buttons in the panel: New Game (left) and Hint (right).
 * 2*BTN_WIDTH + 20 gap = 300 px, centered in the 360 px window. */
Rectangle newGameButton = {.x = 30.0f,
                           .y = ROWS * CELL_SIZE + 15.0f,
                           .width = BTN_WIDTH,
                           .height = BTN_HEIGHT};
Rectangle hintButton = {.x = 30.0f + BTN_WIDTH + 20.0f,
                        .y = ROWS * CELL_SIZE + 15.0f,
                        .width = BTN_WIDTH,
                        .height = BTN_HEIGHT};

/* Most recent hint-request outcome, shown as a transient status message. */
HintResult lastHintResult = HINT_OK;
bool hasHintResult = false;

/* True when the Hint control is usable: live play, not over, budget left. */
static bool hint_enabled(const Game *game) {
  return !game->isReplaying && !game->gameOver && !game->won &&
         game->hintsRemaining > 0;
}

/* Pulsing gold outline around the suggested safe cell. */
static void DrawHintHighlight(const Game *game) {
  if (!game->hasHint)
    return;
  if (game->hintRow < 0 || game->hintRow >= ROWS || game->hintCol < 0 ||
      game->hintCol >= COLUMNS)
    return;

  float t = sinf(game->clock * 5.0f) * 0.5f + 0.5f; // 0..1
  unsigned char alpha = (unsigned char)(150 + (int)(100.0f * t));
  Color c = (Color){255, 215, 0, alpha}; // gold
  Rectangle r = {game->hintCol * CELL_SIZE, game->hintRow * CELL_SIZE,
                 CELL_SIZE, CELL_SIZE};
  DrawRectangleLinesEx(r, 3.0f, c);
}

void DrawUI(const Game *game) {
  int panelY = ROWS * CELL_SIZE;

  DrawRectangle(0, panelY, WINDOW_WIDTH, PANEL_HEIGHT,
                (Color){40, 44, 52, 255});

  Vector2 mouse = GetMousePosition();

  /* New Game button. */
  bool newHovered = CheckCollisionPointRec(mouse, newGameButton);
  Color newColor =
      newHovered ? (Color){90, 200, 250, 255} : (Color){70, 130, 180, 255};
  DrawRectangleRounded(newGameButton, 0.3f, 8, newColor);
  const char *newLabel = "New Game";
  int newLabelWidth = MeasureText(newLabel, 18);
  DrawText(newLabel, newGameButton.x + (newGameButton.width - newLabelWidth) / 2,
           newGameButton.y + (newGameButton.height - 18) / 2, 18, RAYWHITE);

  /* Hint button - shows the budget in the label; greys out when unavailable. */
  bool enabled = hint_enabled(game);
  bool hintHovered = CheckCollisionPointRec(mouse, hintButton);
  Color hintColor;
  if (!enabled) {
    hintColor = (Color){55, 55, 65, 255};
  } else {
    hintColor =
        hintHovered ? (Color){90, 200, 250, 255} : (Color){70, 130, 180, 255};
  }
  DrawRectangleRounded(hintButton, 0.3f, 8, hintColor);
  const char *hintLabel =
      TextFormat("Hint (%d/%d)", game->hintsRemaining, HINT_BUDGET);
  int hintLabelWidth = MeasureText(hintLabel, 18);
  Color hintText = enabled ? RAYWHITE : (Color){140, 140, 150, 255};
  DrawText(hintLabel, hintButton.x + (hintButton.width - hintLabelWidth) / 2,
           hintButton.y + (hintButton.height - 18) / 2, 18, hintText);

  /* Status/message line (centered on the panel, below the buttons). */
  const char *status = NULL;
  Color statusColor = RAYWHITE;
  if (game->won) {
    status = "You Win!";
    statusColor = GREEN;
  } else if (game->gameOver) {
    status = "Game Over!";
    statusColor = RED;
  } else if (game->isReplaying) {
    status = "Replaying... (S: save, L: load)";
    statusColor = SKYBLUE;
  } else if (hasHintResult) {
    switch (lastHintResult) {
      case HINT_EXHAUSTED: {
        status = "No hints remaining";
        statusColor = GRAY;
        break;
      }
      case HINT_INCONSISTENT: {
        status = "Board is inconsistent - hints withheld";
        statusColor = ORANGE;
        break;
      }
      case HINT_STUCK: {
        status = "No guaranteed-safe hint available";
        statusColor = GRAY;
        break;
      }
      case HINT_NO_INFO: {
        status = "Reveal a cell first";
        statusColor = GRAY;
        break;
      }
      default: {
        status = NULL;
        break;
      }
    }
  }

  if (status != NULL) {
    int statusWidth = MeasureText(status, 20);
    DrawText(status, (WINDOW_WIDTH - statusWidth) / 2,
             newGameButton.y + newGameButton.height + 12, 20, statusColor);
  }
}

void DrawMinesweeperGrid(const Game *game) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      int x = j * CELL_SIZE;
      int y = i * CELL_SIZE;

      Cell cell = game->grid[i][j];

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
  DrawHintHighlight(game);
  DrawUI(game);
}
