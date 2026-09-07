#include "raylib.h"

#include "../include/board.h"
#include "../include/config.h"
#include "../include/input.h"
#include "../include/render.h"
#include "../include/replay.h"

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(TARGET_FPS);

  ReplayLogInit(&currentLog);
  ReplayLogInit(&activeReplay);

  NewGame();

  while (!WindowShouldClose()) {
    f32 dt = GetFrameTime();

    HandleInput();

    if (isReplaying) {
      UpdateReplayPlayback(dt);
    } else {
      gameClock += dt;
    }

    if (!gameOver && CheckWin()) {
      won = true;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawMinesweeperGrid();
    EndDrawing();
  }

  ReplayLogFree(&currentLog);
  ReplayLogFree(&activeReplay);

  CloseWindow();
  return 0;
}
