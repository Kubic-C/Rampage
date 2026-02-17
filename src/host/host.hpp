#pragma once

#include <cstdarg>
#include <mutex>
#include "../common/common.hpp"

RAMPAGE_START

class Host final : public IHost {
  struct HostFuncs {
    TraceErrorFunc traceError = nullptr;
    TraceFunc trace = nullptr;
  };

public:
  void log(int ec, const char* format, ...) override {
    std::lock_guard lock(m_mutex);
    va_list args;

    va_start(args, format);
    if (m_funcs.traceError)
      m_funcs.traceError(ec, format, args);
    else
      vprintf(format, args);
    va_end(args);
  }

  void log(const char* format, ...) override {
    std::lock_guard lock(m_mutex);
    va_list args;

    va_start(args, format);
    if (m_funcs.traceError)
      m_funcs.traceError(0, format, args);
    else
      vprintf(format, args);
    va_end(args);
  }

  std::mutex& getHostMutex() override {
    return m_mutex;
  }

  void setLogFuncs(const TraceFunc trace, const TraceErrorFunc traceErr) override {
    std::lock_guard lock(m_mutex);

    m_funcs.trace = trace;
    m_funcs.traceError = traceErr;
  }

  EntityWorld& getWorld() override {
    return *m_world;
  }

  std::string getTitle() override {
    return "Rampage";
  }

  Pipeline& getPipeline() override {
    return m_pipeline;
  }

  bool shouldExit() const override {
    return m_exit;
  }

  void exit() override {
    m_exit = true;
  }

public:
  explicit Host();

  int run();

  NODISCARD Status getStatus() const {
    return m_status;
  }

private:
  std::mutex m_mutex;

  Status m_status;
  volatile bool m_exit = false;

  std::unique_ptr<EntityWorldSerializable> m_world;
  HostFuncs m_funcs;
  Pipeline m_pipeline;

  std::vector<IStaticModule*> m_staticModules;
  Map<std::type_index, IStaticModule*> m_typeMap;

private:
  void sortModulesByDependencies();

  template <typename T>
  void addModule() {
    m_world->addContext<T>(*this);
    m_staticModules.push_back(&m_world->getContext<T>());
    m_typeMap[typeid(T)] = m_staticModules.back();
  }
};

RAMPAGE_END
