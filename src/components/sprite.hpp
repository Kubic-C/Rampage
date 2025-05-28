#pragma once

#include "../ecs/ecs.hpp"
#include "../utility/math.hpp"

constexpr float baseSpriteScale = 0.5f;

enum class WorldLayer : u8 {
  Invalid = 8,
  Bottom = 7,
  Floor = 6,
  Wall = 5,
  Turret = 4,
  Item = 3,
  Res = 2, // RESERVED
  Res2 = 1, // RESERVED
  Top = 0,
};

struct SpriteLayer {
  SpriteLayer() = default;

  SpriteLayer(u16 texIndex, glm::vec2 offset, float rot, WorldLayer layer) :
      texIndex(texIndex), offset(offset), rot(rot), layer(layer) {}

  u16 texIndex = 0;
  glm::vec2 offset = {0.0f, 0.0f};
  float rot = 0.0f;
  WorldLayer layer = WorldLayer::Invalid;
};

struct SpriteComponent {
  static constexpr size_t MaxSpriteLaters = maxNumberBits(3);
  SpriteLayer layers[MaxSpriteLaters];
  u8 layerCount = 0;
  float scaling = 1.0f;

  void addLayer(const SpriteLayer& layer) {
    assert(layerCount < MaxSpriteLaters && "Too many sprite layers!");
    layers[layerCount++] = layer;
    if (layer.layer == WorldLayer::Invalid) {
      layers[layerCount - 1].layer = (WorldLayer)(MaxSpriteLaters - layerCount);
    }
  }

  void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0,
                WorldLayer layer = WorldLayer::Invalid) {
    assert(layerCount < MaxSpriteLaters && "Too many sprite layers!");
    if (layer == WorldLayer::Invalid) {
      layer = (WorldLayer)(MaxSpriteLaters - layerCount);
    }
    layers[layerCount++] = SpriteLayer(texIndex, offset, rot, layer);
  }

  SpriteLayer& getLast() { return layers[layerCount - 1]; }

  const SpriteLayer& operator[](size_t index) const {
    assert(index < layerCount);
    return layers[index];
  }

  const SpriteLayer& get(size_t index) const { return layers[index]; }
};

// The sprite is not part of a tilemap
struct SpriteIndependentTag {
  SpriteIndependentTag() = default;

  SpriteIndependentTag(glz::make_reflectable) {}
};

struct TilePosComponent {
  glm::i16vec2 pos;
  EntityId parent;
};

template <>
struct glz::meta<WorldLayer> {
  static constexpr auto value =
      glz::enumerate("Top", WorldLayer::Top, "Res2", WorldLayer::Res2, "Res", WorldLayer::Res, "Item",
                     WorldLayer::Item, "Turret", WorldLayer::Turret, "Wall", WorldLayer::Wall, "Floor",
                     WorldLayer::Floor, "Bottom", WorldLayer::Bottom, "Invalid", WorldLayer::Invalid);
};

template <>
struct glz::meta<SpriteIndependentTag> {
  static constexpr auto value = glz::object();
};
