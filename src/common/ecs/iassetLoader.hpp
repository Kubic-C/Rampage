#pragma once

#include "iworld.hpp"

RAMPAGE_START

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

  virtual bool loadAssetsFromMemory(IWorld& world, size_t size, const u8* data) = 0;
  virtual bool loadAssetsFromFile(IWorld& world, const char* path) = 0;
  virtual AssetId getAssetIdByName(const std::string& name) = 0;
  virtual EntityId getEntityIdByAssetId(AssetId assetId) = 0;
};

using IAssetLoaderImplPtr = std::unique_ptr<IAssetLoaderImpl>;

class AssetLoader {
public:
  AssetLoader(IWorldPtr world, IAssetLoaderImpl& impl)
    : m_world(world), m_impl(impl) {}

  AssetLoader(IWorldPtr world, const AssetLoader& loader)
    : m_world(world), m_impl(loader.m_impl) {}
    
  bool loadAssetsFromMemory(size_t size, const u8* data) {
    return m_impl.loadAssetsFromMemory(*m_world, size, data);
  } 

  bool loadAssetsFromFile(const char* path) {
    return m_impl.loadAssetsFromFile(*m_world, path);
  }

  AssetId getAssetIdByName(const std::string& name) {
    return m_impl.getAssetIdByName(name);
  }

  EntityId getEntityIdByAssetId(AssetId assetId) {
    return m_impl.getEntityIdByAssetId(assetId);
  }

private:
  IWorldPtr m_world;
  IAssetLoaderImpl& m_impl;
};

RAMPAGE_END