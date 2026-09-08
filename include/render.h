#ifndef RENDER_H
#define RENDER_H

#include "raylib.h"

#include "board.h"

extern Rectangle newGameButton;
extern Rectangle hintButton;

// The outcome of the most recent hint request, shown as a one-line message in
// the panel's status area (persists until the next user action). Set by
// HandleInput; drawn by DrawUI.
extern HintResult lastHintResult;
extern bool hasHintResult;

void DrawMinesweeperGrid(void);
void DrawUI(void);

#endif // RENDER_H
