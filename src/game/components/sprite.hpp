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
  static void serialize(capnp::MessageBuilder& builder, Ref component) {
    auto spriteBuilder = builder.initRoot<Schema::SpriteComponent>();
    auto sprite = component.cast<SpriteComponent>();

    spriteBuilder.setScaling(sprite->scaling);

    // Flatten 2D subSprites into list
    auto listBuilder = spriteBuilder.initSubSprites(
        std::accumulate(sprite->subSprites.begin(), sprite->subSprites.end(), size_t(0),
                        [](size_t sum, const auto& row){ return sum + row.size(); }));

    u32 i = 0;
    u16 y = 0;
    for (const auto& row : sprite->subSprites) {
      u16 x = 0;  // Reset x for each new row
      for (const auto& sub : row) {
        auto subBuilder = listBuilder[i++];
        subBuilder.getGridPos().setX(x);
        subBuilder.getGridPos().setY(y);

        auto layersBuilder = subBuilder.initLayers(sub.layerCount);
        for (u32 j = 0; j < sub.layerCount; j++) {
          auto& layer = sub.layers[j];
          auto layerBuilder = layersBuilder[j];
          layerBuilder.setTexIndex(layer.texIndex);
          layerBuilder.getOffset().setX(layer.offset.x);
          layerBuilder.getOffset().setY(layer.offset.y);
          layerBuilder.setRot(layer.rot);
          layerBuilder.setLayer(static_cast<Schema::WorldLayer>(layer.layer));
        }

        x++;
      }
      y++;
    }
  }

  static void deserialize(capnp::MessageReader& reader, Ref component) {
    auto spriteReader = reader.getRoot<Schema::SpriteComponent>();
    auto sprite = component.cast<SpriteComponent>();

    sprite->scaling = spriteReader.getScaling();
    sprite->subSprites.clear();

    const auto subList = spriteReader.getSubSprites();
    for (auto subReader : subList) {
      u16 row = subReader.getGridPos().getY();
      u16 col = subReader.getGridPos().getX();

      // Make sure your 2D vector has enough rows
      if (sprite->subSprites.size() <= row)
          sprite->subSprites.resize(row + 1);
      if (sprite->subSprites[row].size() <= col)
          sprite->subSprites[row].resize(col + 1);

      // Insert the SubSprite into the correct position
      SubSprite& sub = sprite->subSprites[row][col];

      const auto layersList = subReader.getLayers();
      sub.layerCount = layersList.size();
      for (u32 j = 0; j < layersList.size(); j++) {
        auto layerReader = layersList[j];
        auto& layer = sub.layers[j];  // Use sequential index, not enum value
        layer.texIndex = layerReader.getTexIndex();
        layer.offset.x = layerReader.getOffset().getX();
        layer.offset.y = layerReader.getOffset().getY();
        layer.rot = layerReader.getRot();
        layer.layer = static_cast<WorldLayer>(layerReader.getLayer());
      }
    }
  }

  struct SubSprite {
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

    void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0,
                  WorldLayer layer = WorldLayer::Invalid) {
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

  float scaling = 1.0f;
  // Always in a non-jagged array.
  std::vector<std::vector<SubSprite>> subSprites;
};

// The sprite is not part of a tilemap
struct SpriteIndependentTag : SerializableTag {
  SpriteIndependentTag() = default;

  SpriteIndependentTag(glz::make_reflectable) {}
};

struct TilePosComponent {
  glm::i16vec2 pos;
  EntityId parent;
};

RAMPAGE_END

template <>
struct glz::meta<rmp::WorldLayer> {
  using WorldLayer = rmp::WorldLayer;
  static constexpr auto value =
      glz::enumerate("Top", WorldLayer::Top, "Res2", WorldLayer::Res2, "Res", WorldLayer::Res, "Item",
                     WorldLayer::Item, "Turret", WorldLayer::Turret, "Wall", WorldLayer::Wall, "Floor",
                     WorldLayer::Floor, "Bottom", WorldLayer::Bottom, "Invalid", WorldLayer::Invalid);
};

template <>
struct glz::meta<rmp::SpriteIndependentTag> {
  static constexpr auto value = glz::object();
};
