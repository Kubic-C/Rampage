#define CR_HOST
#include <cr.h>

#include "host.hpp"

RAMPAGE_START

Host::Host(const std::string& moduleFilePath) {
  if(!getModuleList(moduleFilePath, m_modules)) {
    m_status = Status::CriticalError;
    return;
  }

  std::filesystem::remove_all(m_modules.tempDir);
  std::filesystem::create_directory(m_modules.tempDir);

  for (auto& module : m_modules.modules) {
    std::cout << '\t' << module.name << '\n';
    module.isLoaded = cr_plugin_open(module.ctx, module.name.c_str());
    if (!module.isLoaded) {
      std::cout << "^^^ FAILED LOAD ^^^" << '\n';
      continue;
    }
    cr_set_temporary_path(module.ctx, m_modules.tempDir);
    module.ctx.userdata = this;
  }
}

int Host::run() {
  while (!m_exit) {
    for (auto& module : m_modules.modules) {
      if (!module.isLoaded)
        continue;

      cr_plugin& ctx = module.ctx;
      int rcode = cr_plugin_update(ctx);

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

      switch (ctx.failure) {
      case CR_NONE:
      case CR_BAD_IMAGE:
        /* Failure because the module is still being compiled. plugin update will check for us. */
        break;
      case CR_STATE_INVALIDATED:
      case CR_MISALIGN:
        log("%s: Binary error. Recompile.\n", module.name.c_str());
        module.isLoaded = false;
        break;
      case CR_INITIAL_FAILURE:
      case CR_ABORT:
        module.isLoaded = false;
        break;
      case CR_USER:
        log("%s: Return error: %i.", module.name.c_str(), rcode);
        break;
      default:
        log("%s: Unknown/Unhandled error: %i\n", module.name.c_str(), (int)ctx.failure);
        break;
      }
    }
  }

  for (auto& module : m_modules.modules) {
    cr_plugin_close(module.ctx);
  }

  return 0;
}

RAMPAGE_END