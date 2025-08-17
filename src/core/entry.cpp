#include "../common/host.hpp"

int onLoad(HostCtx& ctx) {
  return 0;
}

int onUnload(HostCtx& ctx) {
  return 0;
}

int onUpdate(HostCtx& ctx) {
  HostFuncs& hostFuncs = ctx.funcs;

  hostFuncs.trace("<fgGreen>Core Module 2<reset>\n");

  return 0;
}

static unsigned int ASAN_SAFE CR_STATE version = 1;
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