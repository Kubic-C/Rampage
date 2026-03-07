#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

constexpr float baseSpriteScale = 0.5f;

enum class WorldLayer : u8 {
  Floor = 0,
  Wall = 1,
  Item = 2,
  Top = 3,
  Invalid = 4,
};

static constexpr size_t maxSpriteLayers = static_cast<size_t>(WorldLayer::Invalid);

inline WorldLayer getNextLayer(WorldLayer layer) {
  switch (layer) {
    case WorldLayer::Floor:
      return WorldLayer::Wall;
    case WorldLayer::Wall:
      return WorldLayer::Item;
    case WorldLayer::Item:
      return WorldLayer::Top;
    case WorldLayer::Top:
      return WorldLayer::Invalid;
    default:
      assert(false && "Invalid world layer!");
      return WorldLayer::Invalid;
  }
}

static constexpr float WorldLayerInvalidZ = -1;
static constexpr float WorldLayerFloorZ = 0;
static constexpr float WorldLayerWallZ = 1;
static constexpr float WorldLayerItemZ = 2;
static constexpr float WorldLayerTopZ = 3;

inline float getLayerZ(WorldLayer layer) {
  switch (layer) {
    case WorldLayer::Floor:
      return WorldLayerFloorZ;
    case WorldLayer::Wall:
      return WorldLayerWallZ;
    case WorldLayer::Item:
      return WorldLayerItemZ;
    case WorldLayer::Top:
      return WorldLayerTopZ;
    default:
      assert(false && "Invalid world layer!");
      return WorldLayerInvalidZ;
  }
}

struct SpriteLayer {
  SpriteLayer() = default;

  SpriteLayer(u16 texIndex, glm::vec2 offset, float rot, WorldLayer layer) :
      texIndex(texIndex), offset(offset), rot(rot), layer(layer) {}

  // The texture index of the sprite
  u16 texIndex = 0;
  // The offset of the sprite, also used as the center of rotation.
  glm::vec2 offset = {0.0f, 0.0f};
  // The rotation of the sprite.
  float rot = 0.0f;
  // The layer of the sprite, affects which sprite is drawn on top.
  WorldLayer layer = WorldLayer::Invalid;
};

struct SpriteComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  struct SubSprite {
    SpriteLayer layers[maxSpriteLayers];
    u8 layerCount = 0;

    void addLayer(const SpriteLayer& layer) {
      assert(layerCount < maxSpriteLayers && "Too many sprite layers!");
      layers[layerCount++] = layer;
      if (layer.layer == WorldLayer::Invalid) {
        layers[layerCount - 1].layer = (WorldLayer)(maxSpriteLayers - layerCount);
      }
    }

    void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0,
                  WorldLayer layer = WorldLayer::Invalid) {
      assert(layerCount < maxSpriteLayers && "Too many sprite layers!");
      if (layer == WorldLayer::Invalid) {
        layer = (WorldLayer)(maxSpriteLayers - layerCount);
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

  float scaling = 1.0f;
  // Always in a non-jagged array.
  std::vector<std::vector<SubSprite>> subSprites;
};

RAMPAGE_END
