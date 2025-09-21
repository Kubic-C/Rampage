#pragma once

// Purpose of the file is to hold various global contexts that may be held
// within an EntityWorld

struct AppStats {
  float cumTicks;
  float cumFrames;
  float tps;
  float fps;
};

struct DoExit {
  bool exit = false;
};
