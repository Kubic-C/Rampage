#include "spriteRender.hpp"

#include "../../core/module.hpp"
#include "../../render/module.hpp"
#include "../components/sprite.hpp"
#include "../components/tilemap.hpp"
#include "../systems/tilemap.hpp"

RAMPAGE_START

void submitSprite(const Transform& transform, const SpriteComponent& sprite, Render2D& render) {
  for(const auto& row : sprite.subSprites) {
    for(const SpriteComponent::SubSprite& subSprite : row) {
      for(const SpriteLayer& layer : subSprite.layers) {
        DrawTileCmd cmd;
        cmd.texture = layer.texIndex;
        cmd.localOffset = layer.offset;
        cmd.pos = transform.pos;
        cmd.rot = transform.rot.radians() + layer.rot;
        cmd.z = getLayerZ(layer.layer);
        cmd.size = glm::ivec2(1, 1);
        cmd.scale = sprite.scaling;
        render.submit(cmd);
      }
    }
  }
}

int renderSprites(IWorldPtr world, float dt) {
  auto& render2D = world->getContext<Render2D>();
  WorldViewRect viewRect = render2D.getViewRect();

  auto it = world->getWith(world->set<TransformComponent, SpriteComponent>());
  while (it->hasNext()) {
    EntityPtr entity = it->next();
    if(entity.has<UniqueTileComponent>())
      continue;
    auto transform = entity.get<TransformComponent>();
    auto sprite = entity.get<SpriteComponent>();
    if(!viewRect.intersects(transform->pos, Vec2(tileSize)))
      continue;

    submitSprite(*transform, *sprite, render2D);
  }

  it = world->getWith(world->set<TransformComponent, TilemapComponent>());
  while (it->hasNext()) {
    EntityPtr entity = it->next();
    auto tilemap = entity.get<TilemapComponent>();
    auto tilemapTransform = entity.get<TransformComponent>();

    const TilemapComponent::ChunkLayerList& chunkLayers = tilemap->getChunkLayers();
    for (size_t i = 0; i < TilemapComponent::LayerCount; i++) {
      const auto& layer = chunkLayers[i];
      for (const auto& [chunkPos, chunk] : layer) {
        if (!viewRect.intersects(tilemapTransform->getWorldPoint(getLocalChunkCenter(chunkPos)),
                TileChunk::chunkTileSize))
          continue;

        for (size_t i = 0; i < TileChunk::chunkSize * TileChunk::chunkSize; i++) {
          if (!chunk.hasTile(i))
            continue;

          Vec2 pos;
          const Tile& tile = chunk.getTile(i);
          EntityPtr tileEntity = world->getEntity(tile.tileId);
          auto tileComp = tileEntity.get<TileComponent>();
          if(tile.root != tile.ref.worldPos)
            continue; // this is a subtile
          if(tileEntity.has<MultiTileComponent>()) {
            pos = tileEntity.get<MultiTileComponent>()->calculateCentroid(tile.root);
          } else {
            pos = getLocalTileCenter(
                getTilePosFromChunk(chunkPos, TileChunk::convertFromIndex(i)));
          }
          auto spriteComp = tileEntity.get<SpriteComponent>();
          Transform transform(tilemapTransform->getWorldPoint(pos), tilemapTransform->rot + Rot(getTileDirectionToRadians(tile.rotation)));
          if(!viewRect.intersects(transform.pos, Vec2(tileSize)))
            continue;

          submitSprite(transform, *spriteComp, render2D);
        }
      }
    }
  }

  return 0;
}

bool loadSpriteRender(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  IWorldPtr world = host.getWorld();

  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnRenderStage>(renderSprites);

  return true;
}

RAMPAGE_END
