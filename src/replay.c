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

ReplayLog currentLog = {0};
ReplayLog activeReplay = {0};
bool isReplaying = false;
float gameClock = 0.0f;

static int replayIndex = 0;

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

void ReplayLogPush(ReplayLog *log, EventType type, int row, int col) {
  if (log->count >= log->capacity) {
    log->capacity = (log->capacity == 0) ? 64 : log->capacity * 2;
    log->events =
        realloc(log->events, (size_t)log->capacity * sizeof(ReplayEvent));
  }
  log->events[log->count++] = (ReplayEvent){
      .timeStamp = gameClock,
      .type = (uint8_t)type,
      .row = (uint8_t)row,
      .col = (uint8_t)col,
  };
}

bool SaveReplay(const char *path) {
  if (!firstClick)
    return false; // nothing meaningful to replay yet — no mines placed

  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  ReplayHeader header = {
      .magic = REPLAY_MAGIC,
      .version = REPLAY_VERSION,
      .rows = ROWS,
      .cols = COLUMNS,
      .bombs = (uint16_t)BOMBS,
      .firstClickRow = (uint16_t)firstClickRow,
      .firstClickCol = (uint16_t)firstClickCol,
      .seed = currentSeed,
      .eventCount = (uint32_t)currentLog.count,
  };

  if (fwrite(&header, sizeof(header), 1, f) != 1) {
    fclose(f);
    return false;
  }

  if (currentLog.count > 0 &&
      fwrite(currentLog.events, sizeof(ReplayEvent), (size_t)currentLog.count,
             f) != (size_t)currentLog.count) {
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

bool StartReplayPlayback(const char *path) {
  ReplayHeader header;
  ReplayLog loaded;

  if (!LoadReplayFile(path, &loaded, &header))
    return false;

  ReplayLogFree(&activeReplay);
  activeReplay = loaded;

  InitGrid();
  BOMBS = header.bombs;
  currentSeed = header.seed;
  firstClickRow = header.firstClickRow;
  firstClickCol = header.firstClickCol;

  // Regenerate the exact same mine layout the original game had, by
  // re-seeding PCG32 with the stored seed and re-running the same shuffle
  // with the same "safe" first-click cell.
  pcg32_srandom_r(&rng, currentSeed, 1);
  FisherYatesShuffle(firstClickRow, firstClickCol);

  gameOver = false;
  won = false;
  revealCount = 0;
  firstClick = true; // mines are already placed; PerformReveal must not re-roll
  gameClock = 0.0f;
  replayIndex = 0;
  isReplaying = true;

  return true;
}

void UpdateReplayPlayback(float dt) {
  if (!isReplaying)
    return;

  gameClock += dt;

  while (replayIndex < activeReplay.count &&
         activeReplay.events[replayIndex].timeStamp <= gameClock) {
    ReplayEvent evt = activeReplay.events[replayIndex];
    if (evt.type == EVT_REVEAL) {
      PerformReveal(evt.row, evt.col);
    } else if (evt.type == EVT_TOGGLE_FLAG) {
      PerformToggleFlag(evt.row, evt.col);
    }
    replayIndex++;
  }

  if (!gameOver && CheckWin())
    won = true;

  if (replayIndex >= activeReplay.count) {
    isReplaying = false; // playback done; board stays on final state
  }
}
