#pragma once

#include "utility/ecs.hpp"
#include "components/sprite.hpp"
#include "components/tile.hpp"

typedef u32 AssetId;

class AssetLoader {
  struct SpriteAsset {
    SpriteComponent sprite;
  };

  struct ItemAsset {
    EntityId entity;
  };

  using Asset = std::variant<SpriteAsset, ItemAsset, TileDef>;

public:
  AssetLoader(EntityWorld& world)
    : m_world(world) {}

  bool loadAssets(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
      return false;

    json json = json::parse(file, nullptr, false, true);
    file.close();
    if (json.is_discarded() || !json.is_array())
      return false;

    std::string parentDir = getPath(path);
    for (const auto& element : json) {
      loadAsset(parentDir, element);
    }

    return true;
  }

  bool loadAsset(const std::string& parentDir, const json& json) {
    std::string type = json["type"].get<std::string>();
    std::string name = json["name"].get<std::string>();
   
    if (type == "Sprite") {
      loadSprite(parentDir, name, json);
    } else if (type == "Item") {
      loadItem(parentDir, name, json);
    } else if (type == "Tile") {
      loadTile(parentDir, name, json);
    } else {
      logGeneric("Failed to load asset: invalid type, %s\n", type.c_str());
      return false;
    }

    return true;
  }

  AssetId getAssetId(const std::string& name) {
    return m_assetsByName.find(name)->second;
  }

public:
  SpriteComponent& getSprite(AssetId assetId) {
    return std::get<SpriteAsset>(m_assets.find(assetId)->second).sprite;
  }

  SpriteComponent& getSprite(const std::string& assetName) {
    return std::get<SpriteAsset>(m_assets.find(getAssetId(assetName))->second).sprite;
  }

public:
  TileDef& getTilePrefab(AssetId assetId) {
    return std::get<TileDef>(m_assets.find(assetId)->second);
  }

  TileDef& getTilePrefab(const std::string& assetName) {
    return std::get<TileDef>(m_assets.find(getAssetId(assetName))->second);
  }

  TileDef cloneTilePrefab(AssetId assetId) {
    TileDef& prefab = getTilePrefab(assetId);
    TileDef clone;

    clone = prefab;
    clone.entity = m_world.get(prefab.entity).clone();

    return clone;
  }

  TileDef cloneTilePrefab(const std::string& assetName) {
    return cloneTilePrefab(getAssetId(assetName));
  }

public:
  EntityId& getItemRawId(AssetId assetId) {
    return std::get<ItemAsset>(m_assets.find(assetId)->second).entity;
  }

  EntityId& getItemRawId(const std::string& assetName) {
    return std::get<ItemAsset>(m_assets.find(getAssetId(assetName))->second).entity;
  }

  Entity getItem(AssetId assetId) {
    return m_world.get(getItemRawId(assetId));
  }

  Entity getItem(const std::string& assetName) {
    return m_world.get(getItemRawId(assetName));
  }

private:
  template<typename T>
  AssetId createAsset(const std::string& name) {
    AssetId asset = m_idMgr.generate();

    m_assets[asset].emplace<T>();
    m_assetsByName[name] = asset;

    return asset;
  }

  void loadSprite(const std::string& parentDir, const std::string& name, const json& json);
  void loadItem(const std::string& parentDir, const std::string& name, const json& json);
  void loadTile(const std::string& parentDir, const std::string& name, const json& json);
  
private:
  EntityWorld& m_world;
  IdManager<AssetId> m_idMgr;
  Map<AssetId, Asset> m_assets;
  Map<std::string, AssetId> m_assetsByName;
};