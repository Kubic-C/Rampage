#include "log.hpp"

#include "../common/common.hpp"
#include "../host/host.hpp"

using namespace rmp;

class LogModule final : public DynamicModule {
public:
  LogModule()
    : DynamicModule("LogModule") {}

protected:
  int onLoad(Host& host, u32 version) override {
    logInit();
    host.setLogFuncs(logGeneric, logError);

    host.log("Print dat mufo\n");

    return 0;
  }

  int onUnload(Host& host) override {
    host.setLogFuncs(nullptr, nullptr);

    return 0;
  }

  int onUpdate(Host& host) override {
    return 0;
  }
};

LogModule module;
static u32 ASAN_SAFE CR_STATE version = 1;
CR_EXPORT int cr_main(const cr_plugin* ctx, cr_op op) {
  return module.run(ctx, op, version);
}