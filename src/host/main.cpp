#define CR_HOST
#include <iostream>
#include <filesystem>
#include <fstream>

#include "../common/host.hpp"

bool getModuleList(const std::string& moduleFilePath, ModuleList& modules) {
  const std::string& workingDir = std::filesystem::current_path().string();
  const std::string& fullFilePath = workingDir + "/" + moduleFilePath;
  std::ifstream file(fullFilePath);
  if (!file.is_open()) {
    std::cout << "Failed to load file, does not exist\n";
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
    std::cout << "Failed to load modules:\n\t" << msg << std::endl;
    return false;
  }

  for (auto& module : modules.modules) {
    module.name = workingDir + "\\" + module.name;
    if (!std::filesystem::exists(module.name)) {
      std::cout << module.name << " does not exist\n";
      return false;
    }
  }

  return true;
}

void pause() {
  std::cout << "Press enter to continue...";
  char c;
  std::cin >> c;
}

int main () {
  HostCtx host;
  if (!getModuleList("module.json", host.modules)) {
    std::cout << std::endl;

    pause();
    return 1;
  }

  std::cout << "Loading Modules:\n";
  for (auto& module : host.modules.modules) {
    std::cout << '\t' << module.name << '\n';
    module.isLoaded = cr_plugin_open(module.ctx, module.name.c_str());
    if (!module.isLoaded) {
      std::cout << "^^^ FAILED LOAD ^^^" << '\n';
      continue;
    }
    cr_set_temporary_path(module.ctx, host.modules.tempDir);
    module.ctx.userdata = &host;
  }

  while (!host.exit) {
    for (auto& module : host.modules.modules) {
      if (!module.isLoaded)
        continue;

      cr_plugin& ctx = module.ctx;
      cr_plugin_update(ctx);

      /*
        CR_NONE,     // No error
        CR_SEGFAULT, // SIGSEGV / EXCEPTION_ACCESS_VIOLATION
        CR_ILLEGAL,  // illegal instruction (SIGILL) / EXCEPTION_ILLEGAL_INSTRUCTION
        CR_ABORT,    // abort (SIGBRT)
        CR_MISALIGN, // bus error (SIGBUS) / EXCEPTION_DATATYPE_MISALIGNMENT
        CR_BOUNDS,   // EXCEPTION_ARRAY_BOUNDS_EXCEEDED
        CR_STACKOVERFLOW, // EXCEPTION_STACK_OVERFLOW
        CR_STATE_INVALIDATED, // one or more global data section changed and does
                              // not safely match basically a failure of
                              // cr_plugin_validate_sections
        CR_BAD_IMAGE, // The binary is not valid - compiler is still writing it
        CR_INITIAL_FAILURE, // Plugin version 1 crashed, cannot rollback
        CR_OTHER,    // Unknown or other signal,
        CR_USER = 0x100,
      */

      cr_failure failure = ctx.failure;
      switch (failure) {
      case CR_NONE:
      case CR_BAD_IMAGE:
        /* Failure because the module is still being compiled. plugin update will check for us. */
        break;
      case CR_STATE_INVALIDATED:
      case CR_MISALIGN:
        std::cout << module.name << " binary error, recompile all dependencies and restart the application." << '\n';
        module.isLoaded = false;
        break;
      case CR_INITIAL_FAILURE:
      case CR_ABORT:
        module.isLoaded = false;
        break;
      default:
        std::cout << module.name << ":\n\t Unknown/Unhandled error: " << ctx.failure << "\n";
        break;
      }
    }
  }

  for (auto& module : host.modules.modules) {
    cr_plugin_close(module.ctx);
  }

  pause();
  return 0;
}