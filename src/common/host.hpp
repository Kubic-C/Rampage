#pragma once
#include <mutex>

#include "module.hpp"

typedef void(*TraceErrorFunc)(int ec, const char* format, ...);
typedef void(*TraceFunc)(const char* format, ...);

struct HostFuncs {
  TraceErrorFunc traceError = nullptr;
  TraceFunc trace = nullptr;
};

struct HostCtx {
  std::mutex mutex;
  bool exit = false;
  ModuleList modules;
  HostFuncs funcs;
};