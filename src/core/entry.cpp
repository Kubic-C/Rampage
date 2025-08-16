#include "../common/host.hpp"

int onLoad(HostCtx& ctx) {
  return 0;
}

int onUnload(HostCtx& ctx) {
  return 0;
}

int onUpdate(HostCtx& ctx) {
  HostFuncs& hostFuncs = ctx.funcs;

  hostFuncs.trace("<fgGreen>Core Module");

  return 0;
}

CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
  HostCtx& hostCtx = *static_cast<HostCtx*>(ctx->userdata);
  HostFuncs& hostFuncs = hostCtx.funcs;

  if (ctx->failure != CR_NONE) {
    hostFuncs.traceError(1, "Failed entry in module");
    return -1;
  }

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