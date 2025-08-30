#include "../common/common.hpp"
#include "../host/host.hpp"

using namespace rmp;

class CoreModule final : public DynamicModule {
public:
  CoreModule()
    : DynamicModule("CoreModule") {}

protected:
  int onLoad(Host& host, u32 version) override {
    return 0;
  }

  int onUnload(Host& host) override {
    return 0;
  }

  int onUpdate(Host& host) override {
    return 0;
  }
};

CoreModule module;
static u32 ASAN_SAFE CR_STATE version = 1;
CR_EXPORT int cr_main(const cr_plugin* ctx, cr_op op) {
  return module.run(ctx, op, version);
}