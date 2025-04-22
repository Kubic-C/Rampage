#include "assetLoader.hpp"

#include "inventory.hpp"
#include "turret.hpp"

bool keysExistAndLog(const std::string target, const json& json, const std::vector<std::string>& keys) {
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

b2ShapeDef loadShapeFromJson(const json& json) {
  b2ShapeDef shapeDef = b2DefaultShapeDef();

  if (json.contains("restitution"))
    shapeDef.restitution = json["restitution"].get<float>();
  if (json.contains("friction"))
    shapeDef.friction = json["friction"].get<float>();

  return shapeDef;
}

Entity loadBulletFromJson(EntityWorld& world, const std::string& parentDir, const json& json) {
  SpriteRenderModule& render = world.getModule<SpriteRenderModule>();
  if (!keysExistAndLog("bullet", json, { "damage", "lifetime", "health" }))
    return Entity(world, 0);

  float damage = json["damage"];
  float lifetime = json["lifetime"];
  float health = json["health"];

  Entity e = world.create();
  e.add<ContactDamageComponent>();
  e.add<LifetimeComponent>();
  e.add<TransformComponent>();
  e.add<HealthComponent>();
  ContactDamageComponent& damageComp = e.get<ContactDamageComponent>();
  LifetimeComponent& lifetimeComp = e.get<LifetimeComponent>();
  HealthComponent& healthComp = e.get<HealthComponent>();

  damageComp.damage = damage;
  lifetimeComp.timeLeft = lifetime;
  healthComp.health = health;

  // its essentially a prefab
  e.disable();

  return e;
}

TurretComponent loadTurretFromJson(EntityWorld& world, const std::string& parentDir, const json& json) {
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

void addBreakable(Entity entity, b2ShapeDef& def, json json) {
  if (!keysExistAndLog("breakable", json, { "health" }))
    return;

  entity.add<HealthComponent>();
  HealthComponent& health = entity.get<HealthComponent>();
  health.health = json["health"].get<float>();

  entity.add<ArrowComponent>();
  entity.get<ArrowComponent>().tileCost = health.health;
  entity.add<TileBoundComponent>();
  entity.add<DestroyTileOnEntityRemovalTag>();

  def.enableHitEvents = true;
  def.enableContactEvents = true;
}

void AssetLoader::loadSprite(const std::string& parentDir, const std::string& name, const json& json) {
  AssetId assetId = createAsset<SpriteAsset>(name);
  SpriteComponent& sprite = getSprite(assetId);
  SpriteRenderModule& spriteRender = m_world.getModule<SpriteRenderModule>();

  if (!json.contains("paths")) {
    logGeneric("Failed to load sprite asset! Must contain paths\n");
    return;
  }

  for (const auto& spriteJson : json["paths"]) {
    std::string path = spriteJson["path"].get<std::string>();
    int layer = spriteJson["layer"].get<int>();
    layer = std::min((int)SpriteComponent::MaxSpriteLaters, glm::max(0, layer));

    u32 texIndex = spriteRender.getSpriteFromPath(parentDir + path);
    if (texIndex == UINT32_MAX)
      continue;

    sprite.addLayer(texIndex, Vec2(0), 0, (WorldLayer)layer);
  }
}

void AssetLoader::loadItem(const std::string& parentDir, const std::string& name, const json& json) {
  AssetId assetId = createAsset<ItemAsset>(name);
  EntityId& itemId = getItemRawId(assetId);
  InventoryManager& itemMgr = m_world.getContext<InventoryManager>();

  std::string itemName = json["name"].get<std::string>();
  std::string itemIconPath = parentDir + json["iconPath"].get<std::string>();

  Entity entity = m_world.create();
  if (json.contains("unique") && json["unique"].get<bool>())
    entity.add<ItemAttrUnique>();

  entity.disable();
  entity.add<ItemAttrStackCost>();
  ItemAttrStackCost& itemStackCost = entity.get<ItemAttrStackCost>();
  itemStackCost.stackCost = 1;
  entity.add<ItemAttrIcon>();
  ItemAttrIcon& itemIcon = entity.get<ItemAttrIcon>();
  itemIcon.icon = tgui::Texture(itemIconPath);

  itemId = entity;
}

void AssetLoader::loadTile(const std::string& parentDir, const std::string& name, const json& json) {
  AssetId assetId = createAsset<TileDef>(name);
  TileDef& prefab = getTilePrefab(assetId);

  std::string spriteAssetName = json["sprite"];
  prefab.flags = TileFlags::IS_MAIN_TILE;
  prefab.width = glm::min((i16)maxDim.x, json["size"][0].get<i16>());
  prefab.height = glm::min((i16)maxDim.y, json["size"][1].get<i16>());
  prefab.shapeDef = b2DefaultShapeDef();
  int startingLayer = 0;
  bool needsLocalTransform = false;

  /* Tile Flags */
  for (auto flag : json["flags"])
    prefab.flags |= getTileFlagFromString(flag.get<std::string>());
  if (prefab.flags == std::numeric_limits<u8>::max()) {
    logGeneric("Failed to load tile json. Flag was invalid\n");
    return;
  }
  /* Shape Properties */
  if (json.contains("shape"))
    prefab.shapeDef = loadShapeFromJson(json["shape"]);

  Entity tileEntity = m_world.create();
  tileEntity.disable();
  prefab.entity = tileEntity;

  tileEntity.add<SpriteComponent>();
  tileEntity.get<SpriteComponent>() = getSprite(spriteAssetName);
  if (~prefab.flags & IS_COLLIDABLE)
    tileEntity.add<ArrowComponent>();
  if (json.contains("turret")) {
    tileEntity.add<TurretComponent>();
    tileEntity.get<TurretComponent>() = loadTurretFromJson(m_world, parentDir, json["turret"]);
    tileEntity.add<TransformComponent>(); // in order for turrets to match the turret system they need a transform
  }
  if (json.contains("breakable")) {
    addBreakable(tileEntity, prefab.shapeDef, json["breakable"]);
  }
  if (json.contains("item")) {
    std::string itemName = json["item"].get<std::string>();
    Entity item = getItem(itemName);

    item.add<ItemAttrTile>();
    item.get<ItemAttrTile>().tileId = assetId;
    tileEntity.add<TileItemComponent>();
    tileEntity.get<TileItemComponent>().item = item;
  }
}