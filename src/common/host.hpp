#pragma once
#include <mutex>
#include "module.hpp"

#ifdef __SANITIZE_ADDRESS__
#define ASAN
#endif

#ifdef ASAN
#if defined(__clang__) || defined(__GNUC__)
  #define ASAN_SAFE __attribute__((no_sanitize("address")))
#elif defined(_MSC_VER)
  #define ASAN_SAFE __declspec(no_sanitize_address)
#else
  #define ASAN_SAFE
#endif
#else
#define ASAN_SAFE
#endif

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