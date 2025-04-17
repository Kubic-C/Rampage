#include "tilePrefabs.hpp"

#include "spriteRender.hpp"
#include "item.hpp"
#include "seekPlayer.hpp"

bool TilePrefabs::loadFromFile(const std::string& filePath) {
  std::ifstream file(filePath);
  if (!file.is_open())
    return false;

  json json = json::parse(file, nullptr, false, true);
  file.close();
  if (json.is_discarded() || !json.is_array())
    return false;

  std::string parentDir = getPath(filePath);
  SpriteRenderModule& spriteRender = m_world.getModule<SpriteRenderModule>();
  ItemManager& itemMgr = m_world.getContext<ItemManager>();
  const std::vector<std::string> requiredKeys = {
    "name", "spritePath", "flags", "size"
  };
  for (auto& tileJson : json) {
    for (const std::string& key : requiredKeys) {
      if (!tileJson.contains(key)) {
        logGeneric("Failed to load tile json. No %s\n", key.c_str());
        continue;
      }
    }

    std::string name = tileJson["name"].get<std::string>();
    std::string spritePath = parentDir + tileJson["spritePath"].get<std::string>();
    u8 tileFlags = 0;
    for (auto flag : tileJson["flags"])
      tileFlags |= getTileFlagFromString(flag.get<std::string>());
    if (tileFlags == std::numeric_limits<u8>::max()) {
      logGeneric("Failed to load tile json. Flag was invalid\n");
      continue;
    }

    glm::i16vec2 size = { tileJson["size"][0].get<int>(), tileJson["size"][1].get<int>() };
    if (size.x < 0 || size.y < 0) {
      logGeneric("Failed to load tile json. Size was invalid\n");
      continue;
    }

    Entity tileEntity = m_world.create();
    tileEntity.add<TileSpriteComponent>();
    tileEntity.get<TileSpriteComponent>().texIndex = spriteRender.getSpriteFromPath(spritePath);
    if (~tileFlags & IS_COLLIDABLE)
      tileEntity.add<ArrowComponent>();
    TilePrefabId tilePrefabId = createPrefab(name, tileEntity, tileFlags, size);

    if (tileJson.contains("item")) {
      auto itemJson = tileJson["item"];
      std::string itemName = itemJson["name"].get<std::string>();
      std::string itemIconPath = parentDir + itemJson["iconPath"].get<std::string>();

      Entity itemEntity = m_world.create();
      itemEntity.add<ItemAttrTile>();
      itemEntity.get<ItemAttrTile>().tileId = tilePrefabId;
      itemMgr.createItem(itemName, itemEntity, itemIconPath.c_str());

      tileEntity.add<TileItemComponent>();
      TileItemComponent& tileItem = tileEntity.get<TileItemComponent>();
      tileItem.item = itemEntity;
    }
  }
}