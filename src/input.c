#include "../include/input.h"
#include "raylib.h"

#include <stddef.h>

#include "../include/board.h"
#include "../include/config.h"
#include "../include/render.h"
#include "../include/replay.h"

void HandleInput(Game *game) {
  Vector2 mouse = GetMousePosition();

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      CheckCollisionPointRec(mouse, newGameButton)) {
    GameNew(game);
    hasHintResult = false; // a new game clears any pending hint message
    return;
  }

  if (IsKeyPressed(KEY_S)) {
    if (GameSaveReplay(game, REPLAY_PATH)) {
      TraceLog(LOG_INFO, "Saved replay.msr (%d events)", game->log.count);
    } else {
      TraceLog(LOG_WARNING, "Failed to save replay.msr");
    }
  }

  if (IsKeyPressed(KEY_L)) {
    if (GameStartReplayPlayback(game, REPLAY_PATH)) {
      TraceLog(LOG_INFO, "Replaying replay.msr");
      hasHintResult = false; // a load replaces the board context
    } else {
      TraceLog(LOG_WARNING, "Failed to load replay.msr");
    }
  }

  /* Request a hint: mouse click on the Hint button or the H key. Only legal
   * during active live play (not replay, not after win/loss). */
  bool hintClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                     CheckCollisionPointRec(mouse, hintButton);
  if (hintClicked || IsKeyPressed(KEY_H)) {
    if (!game->isReplaying && !game->gameOver && !game->won) {
      lastHintResult = GameHint(game, NULL, NULL);
      hasHintResult = (lastHintResult != HINT_OK);
    }
  }

  if (game->isReplaying)
    return;
  if (game->gameOver)
    return;

  int col = (int)(mouse.x / CELL_SIZE);
  int row = (int)(mouse.y / CELL_SIZE);

  if (row < 0 || row >= ROWS || col < 0 || col >= COLUMNS)
    return;

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    GameReveal(game, row, col);
    hasHintResult = false; // a move supersedes the transient message
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    GameToggleFlag(game, row, col);
    hasHintResult = false;
  }
}
