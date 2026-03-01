#pragma once

#include "iworld.hpp"
#include "entityPtr.hpp"

RAMPAGE_START

struct AssetTag {};
struct AssetIdType {};
using AssetId = StrongId<AssetIdType>;

RAMPAGE_END

template <>
struct boost::hash<rmp::AssetId> {
  size_t operator()(const rmp::AssetId& assetId) const noexcept {
      return std::hash<rmp::AssetId::IntegerType>{}(assetId.value());
  }
};

RAMPAGE_START

class IAssetLoaderImpl {
public:
  virtual ~IAssetLoaderImpl() = default;

  virtual bool loadAssetsFromString(IWorldPtr world, const std::string& string) = 0;
  virtual bool loadAssetsFromFile(IWorldPtr world, const char* path) = 0;
  virtual AssetId getAssetIdByName(const std::string& name) = 0;
  virtual EntityId getEntityIdByAssetId(AssetId assetId) = 0;
  virtual void registerComponent(IWorldPtr world, ComponentId compId, FromJsonFunc fromJsonFunc) = 0;
  virtual EntityPtr createAsset(IWorldPtr world, const std::string& name) = 0;
};

using IAssetLoaderImplPtr = std::unique_ptr<IAssetLoaderImpl>;

class AssetLoader {
public:
  AssetLoader(IWorldPtr world, IAssetLoaderImpl& impl)
    : m_world(world), m_impl(impl) {}

  AssetLoader(IWorldPtr world, const AssetLoader& loader)
    : m_world(world), m_impl(loader.m_impl) {}
    
  EntityPtr createAsset(const std::string& name) {
    return m_impl.createAsset(m_world, name);
  }

  void registerComponent(ComponentId compId, FromJsonFunc fromJsonFunc) {
    m_impl.registerComponent(m_world, compId, fromJsonFunc);
  }

  bool loadAssetsFromString(const std::string& string) {
    return m_impl.loadAssetsFromString(m_world, string);
  }

  bool loadAssetsFromFile(const char* path) {
    return m_impl.loadAssetsFromFile(m_world, path);
  }

  EntityPtr getAsset(AssetId assetId) {
    return m_world->getEntity(m_impl.getEntityIdByAssetId(assetId));
  }

  EntityPtr getAsset(const std::string& name) {
    return getAsset(m_impl.getAssetIdByName(name));
  }

  EntityPtr cloneAsset(AssetId assetId) {
    return m_world->clone(m_impl.getEntityIdByAssetId(assetId));
  }

  EntityPtr cloneAsset(const std::string& name) {
    return cloneAsset(m_impl.getAssetIdByName(name));
  }

  template<typename T>
  const T& getAssetComponent(AssetId assetId) const {
    return *getAsset(assetId).get<T>();
  }

  template<typename T>
  const T& getAssetComponent(const std::string& name) const {
    return getAssetComponent<T>(m_impl.getAssetIdByName(name));
  }

private:
  IWorldPtr m_world;
  IAssetLoaderImpl& m_impl;
};

RAMPAGE_END