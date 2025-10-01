#pragma once

#include "ecs/ecs.hpp"
#include "imodule.hpp"

RAMPAGE_START

class Pipeline;

typedef void (*TraceErrorFunc)(int ec, const char* format, va_list args);
typedef void (*TraceFunc)(const char* format, va_list args);

class IHost {
public:
  virtual ~IHost() = default;
  virtual void log(int ec, const char* format, ...) = 0;
  virtual void log(const char* format, ...) = 0;
  virtual void setLogFuncs(const TraceFunc trace, const TraceErrorFunc traceErr) = 0;

  virtual EntityWorld& getWorld() = 0;
  virtual std::string getTitle() = 0;
  virtual Pipeline& getPipeline() = 0;
  virtual bool shouldExit() const = 0;
  virtual void exit() = 0;
};

RAMPAGE_END
