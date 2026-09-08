/*
 * ms_tool — headless Minesweeper game-logic driver (no raylib).
 *
 * Links the same board.c / pcg32.c / replay.c the game uses, and lets you
 * script the logic from stdin for debugging and for generating golden output
 * that the test suite can check against.
 *
 *   new [--seed N]     fresh game; --seed N for a deterministic layout
 *   reveal R C         GameReveal(r, c)
 *   flag R C           GameToggleFlag(r, c)
 *   dump [--inspect]   ASCII grid; --inspect also shows hidden mines (M)
 *   state              seed, first-click, counts, flags, replay log lengths
 *   save PATH          GameSaveReplay(PATH)
 *   load PATH          GameStartReplayPlayback(PATH)
 *   step DT            GameUpdateReplayPlayback(DT)
 *   replay PATH [--inspect]  load PATH, step to the end, then dump
 *   hint               solver deductions: safe cells, mines, and a safe hint
 *   quit | EOF         exit
 */
#include "../include/board.h"
#include "../include/config.h"
#include "../include/replay.h"
#include "../include/solver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dump_board(const Game *game, int inspect) {
  printf("grid (%dx%d) bombs=%d:\n", ROWS, COLUMNS, game->bombCount);
  for (int r = 0; r < ROWS; r++) {
    printf("  ");
    for (int c = 0; c < COLUMNS; c++) {
      Cell cell = game->grid[r][c];
      char ch;
      if (cell.flagged) {
        ch = 'F';
      } else if (cell.revealed && cell.hasMines) {
        ch = '*';
      } else if (cell.revealed && cell.neighbourMines > 0) {
        ch = '0' + cell.neighbourMines;
      } else if (cell.revealed) {
        ch = '.';
      } else if (inspect && cell.hasMines) {
        ch = 'M';
      } else {
        ch = '?';
      }
      putchar(ch);
      putchar(' ');
    }
    putchar('\n');
  }
}

static void dump_state(const Game *game) {
  printf("seed=%llu firstClick=%d firstClick=(%d,%d) revealCount=%d bombs=%d "
         "gameOver=%d won=%d replaying=%d clock=%.3f log=%d replay=%d\n",
         (unsigned long long)game->seed, game->firstClick ? 1 : 0,
         game->firstClickRow, game->firstClickCol, game->revealCount,
         game->bombCount, game->gameOver ? 1 : 0, game->won ? 1 : 0,
         game->isReplaying ? 1 : 0, (double)game->clock, game->log.count,
         game->playback.count);
}

static int parse_int(const char *s, int *out) {
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s) {
    return 0;
  }
  *out = (int)v;
  return 1;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  Game game = {0};
  GameNew(&game); /* start with a playable board; `new --seed N` for stable */

  char line[256];
  printf("ms_tool: type a command (new/reveal/flag/dump/state/save/load/step/replay/quit)\n");
  while (1) {
    printf("> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    /* strip trailing newline */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';

    char cmd[256], a1[256], a2[256], a3[256];
    int n = sscanf(line, "%255s %255s %255s %255s", cmd, a1, a2, a3);
    if (n < 1)
      continue;

    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
      break;
    } else if (strcmp(cmd, "new") == 0) {
      if (n >= 3 && strcmp(a1, "--seed") == 0) {
        long long seed = atoll(a2);
        GameNewWithSeed(&game, (u64)seed);
        printf("new game seeded %lld\n", (long long)game.seed);
      } else {
        GameNew(&game);
        printf("new game seeded %llu\n", (unsigned long long)game.seed);
      }
    } else if (strcmp(cmd, "reveal") == 0) {
      int r, c;
      if (n >= 3 && parse_int(a1, &r) && parse_int(a2, &c)) {
        GameReveal(&game, r, c);
        printf("revealed (%d,%d)\n", r, c);
      } else {
        printf("usage: reveal R C\n");
      }
    } else if (strcmp(cmd, "flag") == 0) {
      int r, c;
      if (n >= 3 && parse_int(a1, &r) && parse_int(a2, &c)) {
        GameToggleFlag(&game, r, c);
        printf("toggled flag (%d,%d)\n", r, c);
      } else {
        printf("usage: flag R C\n");
      }
    } else if (strcmp(cmd, "dump") == 0) {
      dump_board(&game, n >= 2 && strcmp(a1, "--inspect") == 0);
    } else if (strcmp(cmd, "state") == 0) {
      dump_state(&game);
    } else if (strcmp(cmd, "save") == 0) {
      if (n >= 2) {
        printf("save %s -> %s\n", a1,
               GameSaveReplay(&game, a1) ? "OK" : "FAIL");
      } else {
        printf("usage: save PATH\n");
      }
    } else if (strcmp(cmd, "load") == 0) {
      if (n >= 2) {
        printf("load %s -> %s\n", a1,
               GameStartReplayPlayback(&game, a1) ? "OK" : "FAIL");
      } else {
        printf("usage: load PATH\n");
      }
    } else if (strcmp(cmd, "step") == 0) {
      double dt = n >= 2 ? atof(a1) : 0.0;
      GameUpdateReplayPlayback(&game, (f32)dt);
      printf("stepped %.3f\n", dt);
    } else if (strcmp(cmd, "replay") == 0) {
      if (n >= 2) {
        int inspect = n >= 3 && strcmp(a2, "--inspect") == 0;
        if (GameStartReplayPlayback(&game, a1)) {
          int guard = 0;
          while (game.isReplaying && guard < 100000) {
            GameUpdateReplayPlayback(&game, 0.05f);
            guard++;
          }
          printf("replay of %s finished (guard=%d)\n", a1, guard);
          dump_board(&game, inspect);
        } else {
          printf("replay %s -> FAIL\n", a1);
        }
      } else {
        printf("usage: replay PATH [--inspect]\n");
      }
    } else if (strcmp(cmd, "hint") == 0) {
      /* One analyse pass: consistency + both deduced sets (ADR-0003). */
      SolverResult r;
      SolverAnalyse(game.grid, &r);
      printf("consistent=%d safe=%d mines=%d\n", r.consistent ? 1 : 0,
             r.safeCount, r.mineCount);
      if (r.consistent) {
        printf("safe:");
        for (int i = 0; i < r.safeCount; i++)
          printf(" (%d,%d)", r.safe[i] / COLUMNS, r.safe[i] % COLUMNS);
        printf("\nmines:");
        for (int i = 0; i < r.mineCount; i++)
          printf(" (%d,%d)", r.mine[i] / COLUMNS, r.mine[i] % COLUMNS);
        printf("\n");
        if (r.safeCount > 0)
          printf("hint -> (%d,%d)\n", r.safe[0] / COLUMNS,
                 r.safe[0] % COLUMNS);
        else
          printf("hint -> none\n");
      } else {
        printf("safe:\n"); /* no lists on an inconsistent board */
        printf("mines:\n");
        printf("hint -> none\n");
      }
    } else {
      printf("unknown command: %s\n", cmd);
    }
  }

  GameFree(&game);
  return 0;
}
