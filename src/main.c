#include "raylib.h"

#include "../include/board.h"
#include "../include/config.h"
#include "../include/input.h"
#include "../include/render.h"
#include "../include/replay.h"

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(TARGET_FPS);

  Game game = {0};
  GameNew(&game);

  while (!WindowShouldClose()) {
    f32 dt = GetFrameTime();

    HandleInput(&game);

    if (game.isReplaying) {
      GameUpdateReplayPlayback(&game, dt);
    } else {
      game.clock += dt;
    }

    if (!game.gameOver && GameCheckWin(&game)) {
      game.won = true;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawMinesweeperGrid(&game);
    EndDrawing();
  }

  GameFree(&game);

  CloseWindow();
  return 0;
}
