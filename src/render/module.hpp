#pragma once
#include "../core/module.hpp"
#include "../log/module.hpp"
#include "render2D/render2D.hpp"
#include "camera.hpp"

RAMPAGE_START

struct RenderGroup {
  struct PreRenderStage {};
  struct ClearWindowStage {};
  struct OnRenderStage {};
  struct OnGUIRenderStage {};
  struct SwapBuffersStage {};
  struct PostRenderStage {};
};

class RenderModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {typeid(LogModule), typeid(CoreModule)};
  }

  explicit RenderModule(IHost& host) : IStaticModule("RenderModule", host) {}

public:
  int onLoad() override;
  int onUpdate() override;

  void enableVsync(bool vsync);
  glm::ivec2 getWindowSize() const;
  bool doesCameraExists() const;
};

RAMPAGE_END
