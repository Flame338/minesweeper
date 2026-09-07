#ifndef CONFIG_H
#define CONFIG_H

#define WINDOW_TITLE "Minesweeper"
#define TARGET_FPS 120

#define ROWS 9
#define COLUMNS 9
#define CELL_SIZE 40

#define PANEL_HEIGHT 90
#define WINDOW_WIDTH (COLUMNS * CELL_SIZE)
#define WINDOW_HEIGHT (ROWS * CELL_SIZE + PANEL_HEIGHT)

#define BTN_WIDTH 140
#define BTN_HEIGHT 40

// Single source of truth for the replay file path. The game saves and loads
// this exact file (see input.c); keeping it in one place prevents the
// save/load path from ever drifting apart.
#define REPLAY_PATH "replay.msr"

#endif // !CONFIG_H
