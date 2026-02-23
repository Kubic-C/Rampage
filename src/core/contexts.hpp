#pragma once

#include "../common/common.hpp"

RAMPAGE_START

// Purpose of the file is to hold various global contexts that may be held
// within an IWorldPtr

struct AppStats {
  float cumTicks;
  float cumFrames;
  float tps;
  float fps;
  u32 tick;
};

struct DoExit {
  bool exit = false;
};

RAMPAGE_END
