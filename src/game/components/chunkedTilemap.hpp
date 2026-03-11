#pragma once

#include "tilemap.hpp"

RAMPAGE_START

struct GeneratorChunkInfo {
  size_t seed;
  EntityId tilemapEntity;
  glm::ivec2 minTilePos; // INCLUSIVE
  glm::ivec2 maxTilePos; // EXCLUSIVE
};

using ChunkGeneratorFunc = void(*)(IWorldPtr world, const GeneratorChunkInfo& info);

struct ChunkLoaderTag : SerializableTag, JsonableTag {};

struct ChunkedTilemapComponent {
  static void serialize(capnp::MessageBuilder& builder, Ref component);
  static void deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component);
  static void fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson);

  // For procedual generation of chunks.
  ChunkGeneratorFunc generator = nullptr;
  // Seed for chunk generation. Can be used to generate the same world consistently.
  size_t seed = 5;
  // loading radius, in chunks.
  size_t loadRadius = 1;
  // The tilemap for each chunk, keyed by chunk coordinates.
  Set<glm::ivec2> chunks; // if contained within set, chunk is already loaded.
  // Loaded chunks
  Set<glm::ivec2> loadedChunks;
};

RAMPAGE_END