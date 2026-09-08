#include "../include/replay.h"

#include <stdio.h>
#include <stdlib.h>

#include "../include/board.h"
#include "../include/config.h"

#define REPLAY_MAGIC 0x5253534Du // "MSSR"-ish tag, just needs to be unique
#define REPLAY_VERSION 2u        // v2: PCG32 seed + first click, no bitmap

// Kept internal to this file — nothing outside replay.c needs the on-disk
// layout directly.
typedef struct {
  uint32_t magic;
  uint32_t version;
  uint16_t rows;
  uint16_t cols;
  uint16_t bombs;
  uint16_t firstClickRow;
  uint16_t firstClickCol;
  uint64_t seed; // PCG32 initstate; regenerates the mine layout exactly
  uint32_t eventCount;
} ReplayHeader;

void ReplayLogInit(ReplayLog *log) {
  log->events = NULL;
  log->count = 0;
  log->capacity = 0;
}

void ReplayLogFree(ReplayLog *log) {
  free(log->events);
  log->events = NULL;
  log->count = 0;
  log->capacity = 0;
}

void ReplayLogPush(ReplayLog *log, f32 timeStamp, EventType type, int row,
                   int col) {
  if (log->count >= log->capacity) {
    log->capacity = (log->capacity == 0) ? 64 : log->capacity * 2;
    log->events =
        realloc(log->events, (size_t)log->capacity * sizeof(ReplayEvent));
  }
  log->events[log->count++] = (ReplayEvent){
      .timeStamp = timeStamp,
      .type = (uint8_t)type,
      .row = (uint8_t)row,
      .col = (uint8_t)col,
  };
}

bool GameSaveReplay(const Game *game, const char *path) {
  if (!game->firstClick)
    return false; // nothing meaningful to replay yet — no mines placed

  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  ReplayHeader header = {
      .magic = REPLAY_MAGIC,
      .version = REPLAY_VERSION,
      .rows = ROWS,
      .cols = COLUMNS,
      .bombs = (uint16_t)game->bombCount,
      .firstClickRow = (uint16_t)game->firstClickRow,
      .firstClickCol = (uint16_t)game->firstClickCol,
      .seed = game->seed,
      .eventCount = (uint32_t)game->log.count,
  };

  if (fwrite(&header, sizeof(header), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (game->log.count > 0 &&
      fwrite(game->log.events, sizeof(ReplayEvent), (size_t)game->log.count,
             f) != (size_t)game->log.count) {
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}

static bool LoadReplayFile(const char *path, ReplayLog *outLog,
                           ReplayHeader *outHeader) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  ReplayHeader header;
  if (fread(&header, sizeof(header), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (header.magic != REPLAY_MAGIC || header.version != REPLAY_VERSION ||
      header.rows != ROWS || header.cols != COLUMNS) {
    fclose(f);
    return false;
  }

  ReplayLogInit(outLog);
  outLog->count = (int)header.eventCount;
  outLog->capacity = (int)header.eventCount;
  if (header.eventCount > 0) {
    outLog->events = malloc(sizeof(ReplayEvent) * header.eventCount);
    if (fread(outLog->events, sizeof(ReplayEvent), header.eventCount, f) !=
        header.eventCount) {
      fclose(f);
      free(outLog->events);
      return false;
    }
  }

  *outHeader = header;
  fclose(f);
  return true;
}

bool GameStartReplayPlayback(Game *game, const char *path) {
  ReplayHeader header;
  ReplayLog loaded;

  if (!LoadReplayFile(path, &loaded, &header))
    return false;

  // A loaded replay is a fresh game context: same single reset as GameNew,
  // then the file's seed / first-click re-create the exact mine layout.
  GameReset(game);
  game->playback = loaded;

  game->bombCount = header.bombs;
  game->seed = header.seed;
  game->firstClickRow = header.firstClickRow;
  game->firstClickCol = header.firstClickCol;

  // Regenerate the exact same mine layout the original game had, by
  // re-seeding PCG32 with the stored seed and re-running the same shuffle
  // with the same "safe" first-click cell.
  pcg32_srandom_r(&game->rng, game->seed, 1);
  GamePlantMines(game, game->firstClickRow, game->firstClickCol);

  game->firstClick = true; // mines are already placed; GameReveal must not
                           // re-roll
  game->clock = 0.0f;
  game->playbackIndex = 0;
  game->isReplaying = true;

  return true;
}

void GameUpdateReplayPlayback(Game *game, f32 dt) {
  if (!game->isReplaying)
    return;

  game->clock += dt;

  while (game->playbackIndex < game->playback.count &&
         game->playback.events[game->playbackIndex].timeStamp <=
             game->clock) {
    ReplayEvent evt = game->playback.events[game->playbackIndex];
    if (evt.type == EVT_REVEAL) {
      GameReveal(game, evt.row, evt.col);
    } else if (evt.type == EVT_TOGGLE_FLAG) {
      GameToggleFlag(game, evt.row, evt.col);
    }
    game->playbackIndex++;
  }

  if (!game->gameOver && GameCheckWin(game))
    game->won = true;

  if (game->playbackIndex >= game->playback.count) {
    game->isReplaying = false; // playback done; board stays on final state
  }
}
