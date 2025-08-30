#pragma once

#include <mutex>
#include "../common/module.hpp"

RAMPAGE_START

typedef void(*TraceErrorFunc)(int ec, const char* format, ...);
typedef void(*TraceFunc)(const char* format, ...);

class Host {
  struct HostFuncs {
    TraceErrorFunc traceError = nullptr;
    TraceFunc trace = nullptr;
  };

  bool getModuleList(const std::string& moduleFilePath, DynamicModuleListMeta& modules) {
    const std::string& workingDir = std::filesystem::current_path().string();
    const std::string& fullFilePath = workingDir + "/" + moduleFilePath;
    std::ifstream file(fullFilePath);
    if (!file.is_open()) {
      log("Failed to load file, does not exist\n");
      return false;
    }

    std::string fileData;
    fileData.resize(std::filesystem::file_size(moduleFilePath));
    file.read(fileData.data(), fileData.size());
    file.close();

    constexpr glz::opts readOps{
      .comments = true,
      .error_on_unknown_keys = true,
      .append_arrays = true,
      .error_on_missing_keys = false // turn this to true for debug reasons
    };

    glz::error_ctx ec = glz::read<readOps>(modules, fileData);
    if (ec) {
      std::string msg = glz::format_error(ec, fileData);
      log("Failed to load modules:\n\t%s\n", msg);
      return false;
    }

    for (auto& module : modules.modules) {
      module.name = workingDir + "\\" + module.name;
      if (!std::filesystem::exists(module.name)) {
        log("%s does not exist\n", module.name);
        return false;
      }
    }

    return true;
  }

public:
  Host(const std::string& moduleFilePath);

  template<class ... Params>
  void log(int ec, const char* format, Params&& ... args) {
    std::lock_guard lock(m_mutex);

    if (m_funcs.traceError)
      m_funcs.traceError(ec, format, args...);
    else
      printf(format, args...);
  }

  template<class ... Params>
  void log(const char* format, Params&& ... args) {
    log(0, format, std::forward<Params>(args)...);
  }

  void setLogFuncs(const TraceFunc trace, const TraceErrorFunc traceErr) {
    std::lock_guard lock(m_mutex);

    m_funcs.trace = trace;
    m_funcs.traceError = traceErr;
  }

  int run();

  Status getStatus() const {
    return m_status;
  }

  bool shouldExit() const {
    return m_exit;
  }

  void exit() {
    m_exit = true;
  }

private:
  std::mutex m_mutex;

  Status m_status;
  volatile bool m_exit = false;

  DynamicModuleListMeta m_modules;
  HostFuncs m_funcs;
};

inline Host& getHost(const cr_plugin* plugin) {
  return *static_cast<Host*>(plugin->userdata);
}

RAMPAGE_END