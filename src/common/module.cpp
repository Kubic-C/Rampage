#include "../host/host.hpp"

RAMPAGE_START

int DynamicModule::run(const cr_plugin *ctx, cr_op operation, u32 version) {
  Host& host = getHost(ctx);

  if (ctx->version < version) {
    host.log(ctx->failure, "<-(CR) Failed entry in module\n");
  }
  version = ctx->version;

  switch (operation) {
  case CR_LOAD:
    host.log("<fgGreen>%s Loaded<reset>\n", m_name.c_str());
    return onLoad(host, version);
  case CR_UNLOAD:
    host.log("<bgRed>%s Unloaded<reset>\n", m_name.c_str());
    return onUnload(host);
  case CR_STEP:
    return onUpdate(host);
  case CR_CLOSE:
    break;
  }

  return -127;
}

RAMPAGE_END