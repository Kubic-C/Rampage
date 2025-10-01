#pragma once

#include "components/components.hpp"
#include "scene.hpp"

RAMPAGE_START

struct AssetJson;

class AssetLoader {
public:
  struct SpriteAsset {
    rmp::SpriteComponent sprite;
  };

  struct PrefabAsset {
    EntityId entity;
  };

  struct SceneAsset {
    std::shared_ptr<Scene> scene;
  };

private:
  using Asset = std::variant<SpriteAsset, PrefabAsset, TileDef, SceneAsset>;

public:
  explicit AssetLoader(EntityWorld& world) : m_world(world) {}

  AssetLoader(AssetLoader&&) = delete;

  bool loadAssets(const std::string& path);
  AssetId loadAsset(const std::string& parentDir, const AssetJson& json);

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
    TileDef clone = prefab;

    clone.entity = m_world.get(prefab.entity).clone();

    return clone;
  }

  TileDef cloneTilePrefab(const std::string& assetName) {
    return cloneTilePrefab(getAssetId(assetName));
  }

public:
  EntityId& getPrefabRawId(AssetId assetId) {
    return std::get<PrefabAsset>(m_assets.find(assetId)->second).entity;
  }

  EntityId& getPrefabRawId(const std::string& assetName) {
    return std::get<PrefabAsset>(m_assets.find(getAssetId(assetName))->second).entity;
  }

  Entity getPrefab(AssetId assetId) {
    return m_world.get(getPrefabRawId(assetId));
  }

  Entity getPrefab(const std::string& assetName) {
    return m_world.get(getPrefabRawId(assetName));
  }

public:
  Scene& getScene(AssetId assetId) {
    return *std::get<SceneAsset>(m_assets.find(assetId)->second).scene;
  }

private:
  template <typename T>
  AssetId createAsset(const std::string& name) {
    AssetId asset = m_idMgr.generate();

    m_assets[asset].emplace<T>();
    m_assetsByName[name] = asset;

    return asset;
  }

  template <typename T>
  AssetId createAsset(const std::string& name, T&& value) {
    AssetId asset = m_idMgr.generate();

    m_assets[asset].emplace<T>(value);
    m_assetsByName[name] = asset;

    return asset;
  }

private:
  EntityWorld& m_world;
  IdManager<AssetId> m_idMgr;
  Map<AssetId, Asset> m_assets;
  Map<std::string, AssetId> m_assetsByName;
};

RAMPAGE_END
