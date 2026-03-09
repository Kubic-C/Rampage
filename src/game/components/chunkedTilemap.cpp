#include "chunkedTilemap.hpp"

RAMPAGE_START

void ChunkedTilemapComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto chunkedTilemapBuilder = builder.getRoot<Schema::ChunkedTilemapComponent>();
  auto self = component.cast<ChunkedTilemapComponent>();

  chunkedTilemapBuilder.setSeed(static_cast<u32>(self->seed));
  chunkedTilemapBuilder.setChunkSize(static_cast<u32>(self->chunkSize));
  chunkedTilemapBuilder.setLoadRadius(static_cast<u32>(self->loadRadius));
  auto chunksBuilder = chunkedTilemapBuilder.initChunks(self->chunks.size());
  size_t i = 0;
  for(const auto& pos : self->chunks) {
    auto chunkBuilder = chunksBuilder[i];
    chunkBuilder.setX(pos.x);
    chunkBuilder.setY(pos.y);
    i++;
  }
  auto loadedChunksBuilder = chunkedTilemapBuilder.initLoadedChunks(self->loadedChunks.size());
  i = 0;
  for(const auto& pos : self->loadedChunks) {
    auto chunkBuilder = loadedChunksBuilder[i];
    chunkBuilder.setX(pos.x);
    chunkBuilder.setY(pos.y);
    i++;
  }
}

void ChunkedTilemapComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto chunkedTilemapReader = reader.getRoot<Schema::ChunkedTilemapComponent>();
  auto self = component.cast<ChunkedTilemapComponent>();

  self->seed = chunkedTilemapReader.getSeed();
  self->chunkSize = chunkedTilemapReader.getChunkSize();
  self->loadRadius = chunkedTilemapReader.getLoadRadius();
  for(const auto& pos : chunkedTilemapReader.getChunks()) {
    self->chunks.insert(glm::ivec2(pos.getX(), pos.getY()));
  }
  for(const auto& pos : chunkedTilemapReader.getLoadedChunks()) {
    self->loadedChunks.insert(glm::ivec2(pos.getX(), pos.getY()));
  }
}

void ChunkedTilemapComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<ChunkedTilemapComponent>();
  auto compJson = jsonValue.as<JSchema::ChunkedTilemapComponent>();

  if(compJson->hasSeed()) {
    self->seed = compJson->getSeed();
  }
  if(compJson->hasChunkSize()) {
    self->chunkSize = compJson->getChunkSize();
  }
  if(compJson->hasLoadRadius()) {
    self->loadRadius = compJson->getLoadRadius();
  }
}

RAMPAGE_END