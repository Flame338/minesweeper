#include "../include/input.h"
#include "raylib.h"

#include "../include/board.h"
#include "../include/config.h"
#include "../include/render.h"
#include "../include/replay.h"

void HandleInput(void) {
  Vector2 mouse = GetMousePosition();

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      CheckCollisionPointRec(mouse, newGameButton)) {
    NewGame();
    return;
  }

  if (IsKeyPressed(KEY_S)) {
    if (SaveReplay("replay.msr")) {
      TraceLog(LOG_INFO, "Saved replay.msr (%d events)", currentLog.count);
    } else {
      TraceLog(LOG_WARNING, "Failed to save replay.msr");
    }
  }

  if (IsKeyPressed(KEY_L)) {
    if (StartReplayPlayback("reoplay.msr")) {
      TraceLog(LOG_INFO, "Replaying replay.msr");
    } else {
      TraceLog(LOG_WARNING, "Failed to load replay.msr");
    }
  }

  if (isReplaying)
    return;
  if (gameOver)
    return;

  int col = (int)(mouse.x / CELL_SIZE);
  int row = (int)(mouse.y / CELL_SIZE);

  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    PerformReveal(row, col);
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    PerformToggleFlag(row, col);
  }
}
