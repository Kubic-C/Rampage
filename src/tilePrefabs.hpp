#pragma once

#include "tile.hpp"

using TilePrefabId = u32;

class TilePrefabs {
public:
  TilePrefabs(EntityWorld& world)
    : m_world(world) {}

  TilePrefabId createPrefab(const std::string& name, Entity entity, u8 tileFlags, const glm::i16vec2& dim) {
    TilePrefabId id = m_idMgr.generate();
    m_prefabNames[name] = id;

    tileFlags |= TileFlags::IS_MAIN_TILE;

    entity.disable();

    Tile tile;
    tile.shapeId = b2_nullShapeId;
    tile.entity = entity;
    tile.flags = tileFlags;
    tile.width = dim.x;
    tile.height = dim.y;

    m_prefabs[id] = tile;

    return id;
  }

  Tile clonePrefab(TilePrefabId id) {
    Tile& prefab = m_prefabs.at(id);
    Tile clone;

    clone = prefab;
    clone.entity = m_world.get(prefab.entity).clone();

    return clone;
  }

  Tile clonePrefab(const std::string& name) {
    assert(m_prefabNames.contains(name));
    return clonePrefab(m_prefabNames.at(name));
  }

private:
  EntityWorld& m_world;
  IdManager<TilePrefabId> m_idMgr;
  Map<TilePrefabId, Tile> m_prefabs;
  Map<std::string, TilePrefabId> m_prefabNames;
};