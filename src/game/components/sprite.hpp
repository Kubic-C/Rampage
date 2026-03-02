#pragma once

#include "../../common/common.hpp"

RAMPAGE_START

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
  static void fromJson(Ref component, AssetLoader loader, const json& compJson);

  struct SubSprite {
    static constexpr size_t MaxSpriteLayers = maxNumberBits(3);
    SpriteLayer layers[MaxSpriteLayers];
    u8 layerCount = 0;

    void addLayer(const SpriteLayer& layer) {
      assert(layerCount < MaxSpriteLayers && "Too many sprite layers!");
      layers[layerCount++] = layer;
      if (layer.layer == WorldLayer::Invalid) {
        layers[layerCount - 1].layer = (WorldLayer)(MaxSpriteLayers - layerCount);
      }
    }

    void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0,
                  WorldLayer layer = WorldLayer::Invalid) {
      assert(layerCount < MaxSpriteLayers && "Too many sprite layers!");
      if (layer == WorldLayer::Invalid) {
        layer = (WorldLayer)(MaxSpriteLayers - layerCount);
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
