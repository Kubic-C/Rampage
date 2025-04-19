#include "tilePrefabs.hpp"

#include "spriteRender.hpp"
#include "item.hpp"
#include "seekPlayer.hpp"
#include "turret.hpp"

bool keysExistAndLog(const std::string target, json& json, const std::vector<std::string>& keys) {
  bool failed = false;
  for (const std::string& key : keys) {
    if (!json.contains(key)) {
      logGeneric("Failed to %s from json. No %s\n", target.c_str(), key.c_str());
      failed = true;
      continue;
    }
  }

  return !failed;
}

SpriteComponent loadSpriteFromPaths(SpriteRenderModule& render, const std::string& parentDir, const std::vector<std::string>& paths) {
  SpriteComponent sprite;
  for (u8 i = 0; i < paths.size(); i++)
    sprite.addLayer(render.getSpriteFromPath(parentDir + paths[i]));
  return sprite;
}

b2ShapeDef loadShapeFromJson(json& json) {
  b2ShapeDef shapeDef = b2DefaultShapeDef();

  if (json.contains("restitution"))
    shapeDef.restitution = json["restitution"].get<float>();
  if (json.contains("friction"))
    shapeDef.friction = json["friction"].get<float>();

  std::cout << shapeDef.friction << '\n';

  return shapeDef;
}

Entity loadBulletFromJson(EntityWorld& world, const std::string& parentDir, json& json) {
  SpriteRenderModule& render = world.getModule<SpriteRenderModule>();
  if(!keysExistAndLog("bullet", json, {"damage", "lifetime", "health"}))
    return Entity(world, 0);

  float damage = json["damage"];
  float lifetime = json["lifetime"];
  float health = json["health"];

  Entity e = world.create();
  e.add<DamageComponent>();
  e.add<LifetimeComponent>();
  e.add<TransformComponent>();
  e.add<HealthComponent>();
  DamageComponent& damageComp = e.get<DamageComponent>();
  LifetimeComponent& lifetimeComp = e.get<LifetimeComponent>();
  HealthComponent& healthComp = e.get<HealthComponent>();

  damageComp.damage = damage;
  lifetimeComp.timeLeft = lifetime;
  healthComp.health = health;
  
  // its essentially a prefab
  e.disable();

  return e;
}

TurretComponent loadTurretFromJson(EntityWorld& world, const std::string& parentDir, json& json) {
  TurretComponent turret;

  if (!keysExistAndLog("turret", json,
      { "bullet",
      "fireRate",
      "radius",
      "turnSpeed",
      "shootRange",
      "stopRange",
      "muzzleVelocity",
      "bulletRadius" }))
    return turret;

  turret.summon = loadBulletFromJson(world, parentDir, json["bullet"]);
  turret.fireRate = json["fireRate"].get<float>();
  turret.radius = json["radius"].get<float>();
  turret.turnSpeed = json["turnSpeed"].get<float>();
  turret.shootRange = json["shootRange"].get<float>();
  turret.stopRange = json["stopRange"].get<float>();
  turret.muzzleVelocity = json["muzzleVelocity"].get<float>();
  turret.bulletRadius = json["bulletRadius"].get<float>();

  return turret;
}

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
    "name", "flags", "size", "spritePaths"
  };
  for (auto& tileJson : json) {
    for (const std::string& key : requiredKeys) {
      if (!tileJson.contains(key)) {
        logGeneric("Failed to load tile json. No %s\n", key.c_str());
        continue;
      }
    }

    std::string name = tileJson["name"].get<std::string>();
    std::vector<std::string> spritePaths = tileJson["spritePaths"];
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
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    if(tileJson.contains("shape"))
      shapeDef = loadShapeFromJson(tileJson["shape"]);

    Entity tileEntity = m_world.create();
    tileEntity.add<SpriteComponent>();
    SpriteComponent& sprite = tileEntity.get<SpriteComponent>();
    sprite = loadSpriteFromPaths(spriteRender, parentDir, spritePaths);
    if (~tileFlags & IS_COLLIDABLE)
      tileEntity.add<ArrowComponent>();
    bool needsLocalTransform = false;
    if (tileJson.contains("turret")) {
      tileEntity.add<TurretComponent>();
      tileEntity.get<TurretComponent>() = loadTurretFromJson(m_world, parentDir, tileJson["turret"]);
      tileEntity.add<TransformComponent>(); // in order for turrets to match the turret system they need a transform
    }
    TilePrefabId tilePrefabId = createPrefab(name, tileEntity, tileFlags, size);

    if (tileJson.contains("item")) {
      auto itemJson = tileJson["item"];
      std::string itemName = itemJson["name"].get<std::string>();
      std::string itemIconPath = parentDir + itemJson["iconPath"].get<std::string>();

      Entity itemEntity = m_world.create();
      itemEntity.add<ItemAttrTile>();
      itemEntity.get<ItemAttrTile>().tileId = tilePrefabId;
      itemMgr.createItem(itemName, itemEntity, itemIconPath.c_str());

      if (itemJson.contains("unique") && itemJson["unique"].get<bool>())
        itemEntity.add<ItemAttrUnique>();

      tileEntity.add<TileItemComponent>();
      TileItemComponent& tileItem = tileEntity.get<TileItemComponent>();
      tileItem.item = itemEntity;
    }
  }
}