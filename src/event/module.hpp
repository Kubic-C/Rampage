#pragma once
#include "../log/module.hpp"
#include "../render/module.hpp"
#include "keys.hpp"

/* DO NOT EXPOSE UNDERLYING MODULE IMPLEMENTATION IN THIS FILE. */

RAMPAGE_START

class EventModule final : public IStaticModule {
public:
  std::vector<std::type_index> getDependencies() override {
    return {typeid(LogModule), typeid(RenderModule), typeid(CoreModule)};
  }

  explicit EventModule(IHost& host) : IStaticModule("EventModule", host) {}

  NODISCARD bool isKeyPressed(Key key) const;
  NODISCARD bool isKeyHeld(Key key) const;
  NODISCARD bool hasWindowResized() const;
  NODISCARD glm::vec2 getWindowSize() const;
  NODISCARD Vec2 getMouseCoords() const;

public:
  int onLoad() override;
  int onUpdate() override;
};

RAMPAGE_END
