#pragma once
#include "../log/module.hpp"
#include "../core/module.hpp"

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
    return { typeid(LogModule), typeid(CoreModule) };
  }

  explicit RenderModule(IHost& host)
    : IStaticModule("RenderModule", host) {}

public:
  int onLoad() override;
  int onUnload() override;
  int onUpdate() override;

  void enableVsync(bool vsync);
  glm::mat4 getProj() const;
  glm::mat4 getView() const;
  glm::ivec2 getWindowSize() const;
  glm::vec2 getWorldCoords(const glm::ivec2& screenCoords) const;
  glm::vec2 getScreenCoords(const glm::ivec2& worldCoords) const;
};

RAMPAGE_END