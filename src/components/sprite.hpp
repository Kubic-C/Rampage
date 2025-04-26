#pragma once

#include "../ecs/ecs.hpp"
#include "../utility/math.hpp"

enum class WorldLayer : u8 {
  Invalid = 8,
  Bottom = 7,
  Floor = 6,
  Wall = 5,
  Turret = 4,
  Item = 3,
  Res = 2,  // RESERVED
  Res2 = 1, // RESERVED
  Top = 0,
};

struct SpriteLayer {
  SpriteLayer() = default;
  SpriteLayer(u16 texIndex, glm::vec2 offset, float rot, WorldLayer layer)
    : texIndex(texIndex), offset(offset), rot(rot), layer(layer) {
  }

  u16 texIndex = 0;
  glm::vec2 offset = { 0.0f, 0.0f };
  float rot = 0.0f;
  WorldLayer layer = WorldLayer::Invalid;
};

struct SpriteComponent {
  static constexpr size_t MaxSpriteLaters = maxNumberBits(3);
  SpriteLayer layers[MaxSpriteLaters];
  u8 layerCount = 0;

  void addLayer(const SpriteLayer& layer) {
    assert(layerCount < MaxSpriteLaters && "Too many sprite layers!");
    layers[layerCount++] = layer;
    if (layer.layer == WorldLayer::Invalid) {
      layers[layerCount - 1].layer = (WorldLayer)(MaxSpriteLaters - layerCount);
    }
  }

  void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0, WorldLayer layer = WorldLayer::Invalid) {
    assert(layerCount < MaxSpriteLaters && "Too many sprite layers!");
    if (layer == WorldLayer::Invalid) {
      layer = (WorldLayer)(MaxSpriteLaters - layerCount);
    }
    layers[layerCount++] = SpriteLayer(texIndex, offset, rot, layer);
  }

  SpriteLayer& getLast() {
    return layers[layerCount - 1];
  }

  const SpriteLayer& operator[](size_t index) const {
    assert(index < layerCount);
    return layers[index];
  }

  const SpriteLayer& get(size_t index) const {
    return layers[index];
  }
};

struct TilePosComponent {
  glm::i16vec2 pos;
  EntityId parent;
};