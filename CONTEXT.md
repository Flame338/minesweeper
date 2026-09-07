# Minesweeper

A Minesweeper clone where the board is built on a deterministic mine layout
(seeded once per game), and where a player's moves can be captured and replayed
exactly. The core lives in `board.c` / `pcg32.c` / `replay.c`, which are
raylib-free so the rules can be tested and scripted headlessly.

## Language

**Board**:
The fixed 9×9 arrangement of Cells. One board per game.

**Cell**:
A single square on the board. It has three independent facts: whether it is
revealed, whether it is flagged, and whether it holds a Mine.
_Avoid_: tile, square, box

**Mine**:
A Cell that, when revealed, ends the game in a loss.
_Avoid_: bomb

**Reveal**:
The act of showing a Cell's contents. Revealing an empty Cell triggers Flood
fill; revealing a Mine ends the game.
_Avoid_: uncover, expose, open

**Flag**:
Marking a hidden Cell as a suspected Mine. A flagged Cell cannot be Revealed.
_Avoid_: mark, marker

**Flood fill**:
When a Cell with a Neighbour count of 0 is Revealed, the connected region of
similarly empty Cells is Revealed too, stopping at the numbered boundary Cells.
_Avoid_: cascade, chain

**Neighbour count**:
How many of a Cell's eight surrounding Cells are Mines. Determines the number
shown when a Cell is Revealed.
_Avoid_: adjacent-count, number

**First click**:
The first Reveal of a game. That Cell is guaranteed not to be a Mine, and it
plants the Mines for the whole board.
_Avoid_: opening move, initial reveal

**Seed**:
The value that, together with the First-click Cell, deterministically fixes the
Mine layout for a game. Same seed + same first click ⇒ same board, every time.
_Avoid_: random seed

**New game**:
Resetting the Board, clearing the recorded moves, and rolling a fresh Seed.
_Avoid_: restart, reset

**Win / Lose**:
The game is won when every non-Mine Cell has been Revealed; lost when a Mine is
Revealed.
_Avoid_: victory, game over (use Lose)

**Replay log**:
The ordered sequence of moves a player made. Capturing these is what makes a
game reproducible.
_Avoid_: history, move list

**Replay event**:
One recorded move — a Reveal or a Flag at a specific Cell, with the time it
happened. Replays store intent (which Cell was touched), never raw input.
_Avoid_: action, move

**Save replay**:
Persisting a game's Seed, First-click Cell, and Replay log to disk.
_Avoid_: export, record

**Load replay (playback)**:
Reading a saved game, regenerating its Mine layout from the Seed, then
Re-applying its Replay events to reconstruct the original board exactly.
_Avoid_: import, replay file
