#include "../common/host.hpp"
#include "log.hpp"

int onLoad(HostCtx& host) {
  std::lock_guard lock(host.mutex);
  HostFuncs& funcs = host.funcs;

  logInit();

  funcs.trace = logGeneric;
  funcs.traceError = logError;

  return 0;
}

int onUnload(HostCtx& host) {
  std::lock_guard lock(host.mutex);
  HostFuncs& funcs = host.funcs;

  funcs.trace = nullptr;
  funcs.traceError = nullptr;

  return 0;
}

int onUpdate(HostCtx& host) {
  return 0;
}

CR_STATE ASAN_SAFE static unsigned int version = 1;
CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
  HostCtx& hostCtx = *static_cast<HostCtx*>(ctx->userdata);
  HostFuncs& hostFuncs = hostCtx.funcs;

  if (ctx->version < version) {
    hostFuncs.traceError(ctx->failure, "<-(CR) Failed entry in module\n");
  }
  version = ctx->version;

  switch (operation) {
  case CR_LOAD:
    return onLoad(hostCtx);
  case CR_UNLOAD:
    return onUnload(hostCtx);
  case CR_STEP:
    return onUpdate(hostCtx);
  case CR_CLOSE:
    break;
  }

  return -127;
}