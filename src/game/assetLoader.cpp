#include "assetLoader.hpp"

#include "inventory.hpp"

// Aint no way, im type all dat out. (THANK YOU)
using namespace rmp;

//////////////////// Sprite Prototype

struct SpritePrototypeLayer {
  std::string path;
  WorldLayer layer = WorldLayer::Invalid;
};

template <>
struct glz::meta<SpritePrototypeLayer> {
  using T = SpritePrototypeLayer;
  static constexpr auto value = object("path", &T::path, "layer", &T::layer);
};

struct SubSpritePrototype {
  int x = 0;
  int y = 0;
  std::vector<SpritePrototypeLayer> layers;
};

template <>
struct glz::meta<SubSpritePrototype> {
  using T = SubSpritePrototype;
  static constexpr auto value = object("x", &T::x, "y", &T::y, "layers", &T::layers);
};

struct SpritePrototype {
  int dimX = 1;
  int dimY = 1;
  float scale = 1.0f;
  std::vector<SubSpritePrototype> subSprites;
};

template <>
struct glz::meta<SpritePrototype> {
  using T = SpritePrototype;
  static constexpr auto value = object("dimX", &T::dimX, "dimY", &T::dimY, "scale", &T::scale, "subSprites", &T::subSprites);
};

//////////////////// SpriteName

struct SpriteName {
  std::string sprite;
  float scale = -1.0f;
};

template <>
struct glz::meta<SpriteName> {
  using T = SpriteName;
  static constexpr auto value = object("name", &T::sprite, "scale", &T::scale);
};

//////////////////// ItemName

struct ItemName {
  std::string item;
};

template <>
struct glz::meta<ItemName> {
  using T = ItemName;
  static constexpr auto value = object("name", &T::item);
};

//////////////////// Item Icon

struct ItemIconPath {
  std::string icon;
};

template <>
struct glz::meta<ItemIconPath> {
  using T = ItemIconPath;
  static constexpr auto value = object("iconPath", &T::icon);
};

//////////////////// Tile additional

struct TileBreakable {
  float health = 1.0f;
};

template <>
struct glz::meta<TileBreakable> {
  using T = TileBreakable;
  static constexpr auto value = object(&T::health);
};

//////////////////// Transform Prototype

struct TransformPrototype {
  Vec2 pos;
  float rot; // Store rotation as radians for serialization

  TransformPrototype() = default;

  TransformPrototype(const Transform& transform) : pos(transform.pos), rot(transform.rot.radians()) {}

  Transform toTransform() const { return Transform(pos, Rot(rot)); }
};

template <>
struct glz::meta<TransformPrototype> {
  using T = TransformPrototype;
  static constexpr auto value = object("pos", &T::pos, "rot", &T::rot);
};

//////////////////// Shape Prototypes

static const std::unordered_map<std::string, uint64_t> categoryMapping = {
    {"Friendly", Friendly}, {"Enemy", Enemy}, {"Static", Static}, {"All", All}};

inline uint64_t convertToBitmask(const std::vector<std::string>& mask) {
  uint64_t bitmask = 0;
  for (const auto& category : mask) {
    auto it = categoryMapping.find(category);
    if (it != categoryMapping.end()) {
      bitmask |= it->second;
    } else {
      // TODO: logGeneric("Unknown mask: %s\n", category.c_str());
    }
  }
  return bitmask;
}

struct AddShapePrototype {
  std::vector<std::string> categoryMask;
  std::vector<std::string> collisionMask;
  b2ShapeDef def = b2DefaultShapeDef();
  ShapeVariant shape;
};

template <>
struct glz::meta<AddShapePrototype> {
  using T = AddShapePrototype;
  static constexpr auto value = object("categoryMask", &T::categoryMask, "collisionMask", &T::collisionMask,
                                       "def", &T::def, "shape", &T::shape);
};

//////////////////// Prefab Prototype

/* The below components are valid to read from json*/

using ComponentPrototype = std::variant<SpriteName, // 1
                                        ItemName, // 2
                                        ItemIconPath, // 3
                                        AddShapePrototype, // 4
                                        HealthComponent, // 5
                                        ContactDamageComponent, // 6
                                        SeekPrimaryTargetTag, // 7
                                        TurretComponent, // 8
                                        ItemAttrStackCost, // 9
                                        ItemAttrUnique, // 10
                                        TileBreakable, // 11
                                        TransformPrototype, // 12
                                        SpriteIndependentTag, // 13
                                        AddBodyComponent // 14
                                        >;

template <>
struct glz::meta<ComponentPrototype> {
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = std::array{
      "spriteName", // 1
      "itemName", // 2
      "itemIcon", // 3
      "addShape", // 4
      "health", // 5
      "contactDamage", // 6
      "seekTarget", // 7
      "turret", // 8
      "stackCost", // 9
      "unique", // 10
      "breakable", // 11
      "transform", // 12
      "spriteIndependent", // 13
      "addRigidBody" // 14
  };
};

struct PrefabPrototype {
  std::vector<ComponentPrototype> comps;
};

template <>
struct glz::meta<PrefabPrototype> {
  using T = PrefabPrototype;
  static constexpr auto value = object("comps", &T::comps);
};

//////////////////// Tile Prototype

struct TilePrototype {
  PrefabPrototype entityComponents;
  TileDef tile;
};

template <>
struct glz::meta<TilePrototype> {
  using T = TilePrototype;
  static constexpr auto value = object("entity", &T::entityComponents, "tile", &T::tile);
};

//////////////////// Scene Asset Prototype

struct SceneAssetPrototype {
  std::string basePrefab;
  int count = 1;
  PrefabPrototype override;
};

template <>
struct glz::meta<SceneAssetPrototype> {
  using T = SceneAssetPrototype;
  static constexpr auto value =
      object("basePrefab", &T::basePrefab, "count", &T::count, "override", &T::override);
};

//////////////////// Scene Prototype

struct ScenePrototype {
  std::string name;
  std::string desc;
  std::vector<std::string> load;
  std::vector<SceneAssetPrototype> scene;
};

template <>
struct glz::meta<ScenePrototype> {
  using T = ScenePrototype;
  static constexpr auto value =
      object("name", &T::name, "desc", &T::desc, "load", &T::load, "scene", &T::scene);
};

//////////////////// Asset Json

// An asset prototype is an asset whose member values have not been verified
// yet, and had not yet been given a AssetId
using AssetPrototype = std::variant<SpritePrototype, PrefabPrototype, TilePrototype, ScenePrototype>;

template <>
struct glz::meta<AssetPrototype> {
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = std::array{"spriteLoad", "prefab", "tileDef", "scene"};
};

RAMPAGE_START

struct AssetJson {
  std::string name;
  AssetPrototype prototype;
};

RAMPAGE_END


template <>
struct glz::meta<AssetJson> {
  using T = AssetJson;
  static constexpr auto value = object("name", &T::name, "data", &T::prototype);
};

RAMPAGE_START

AssetLoader::SpriteAsset loadSprite(EntityWorld& world, const std::string& path,
                                    const SpritePrototype& spriteProto) {
  //TODO: auto& render = world.getModule<SpriteRenderModule>();
  SpriteComponent sprites;
  //TODO:  sprites.subSprites.resize(spriteProto.dimY);
  //TODO: for (auto& col : sprites.subSprites) {
  //TODO:   col.resize(spriteProto.dimX);
  //TODO:  }
  //TODO:  sprites.scaling = spriteProto.scale;
  //TODO:
  //TODO:  bool isSingle = spriteProto.dimX == 1 && spriteProto.dimY == 1;
  //TODO:  for (const SubSpritePrototype& subSpriteProto : spriteProto.subSprites) {
  //TODO:    SpriteComponent::SubSprite& sprite = sprites.subSprites[subSpriteProto.y][subSpriteProto.x];
  //TODO:
  //TODO:    for (const SpritePrototypeLayer& layer : subSpriteProto.layers) {
  //TODO:      u32 index = render.getSpriteFromPath(path + layer.path);
  //TODO:
  //TODO:      if (isSingle)
  //TODO:        sprite.addLayer(index, glm::vec2(0, 0), 0, layer.layer);
  //TODO:      else
  //TODO:        sprite.addLayer(index, (glm::vec2(subSpriteProto.x, subSpriteProto.y) - tileSize) * tileSize, 0, layer.layer);
  //TODO:    }
  //TODO:  }

  return AssetLoader::SpriteAsset(sprites);
}

AssetLoader::PrefabAsset loadPrefabPrototype(AssetLoader& loader, EntityWorld& world, const std::string& path,
                                             AssetId id, const PrefabPrototype& prototypeComponents) {
  Entity e = world.create();
  e.disable();

  for (const ComponentPrototype& proto : prototypeComponents.comps) {
    switch (proto.index()) {
    case IndexInVariant<ComponentPrototype, SpriteName>:
      {
        e.add<SpriteComponent>();
        *e.get<SpriteComponent>() = loader.getSprite(std::get<SpriteName>(proto).sprite);
        float overloadedScale = std::get<SpriteName>(proto).scale;
        if (overloadedScale > 0.0f)
          e.get<SpriteComponent>()->scaling = overloadedScale;
      }
      break;
    case IndexInVariant<ComponentPrototype, AddShapePrototype>:
      {
        e.add<AddShapeComponent>();
        const auto& defProto = std::get<AddShapePrototype>(proto);
        RefT<AddShapeComponent> def = e.get<AddShapeComponent>();

        def->def = defProto.def;
        def->def.filter.categoryBits = convertToBitmask(defProto.categoryMask);
        def->def.filter.maskBits = convertToBitmask(defProto.collisionMask);
        def->shape = defProto.shape;
      }
      break;
    case IndexInVariant<ComponentPrototype, HealthComponent>:
      {
        e.add<HealthComponent>();
        *e.get<HealthComponent>() = std::get<HealthComponent>(proto);
      }
      break;
    case IndexInVariant<ComponentPrototype, ContactDamageComponent>:
      {
        e.add<ContactDamageComponent>();
        *e.get<ContactDamageComponent>() = std::get<ContactDamageComponent>(proto);
      }
      break;
    case IndexInVariant<ComponentPrototype, SeekPrimaryTargetTag>:
      {
        e.add<SeekPrimaryTargetTag>();
      }
      break;
    case IndexInVariant<ComponentPrototype, TurretComponent>:
      {
        e.add<TurretComponent>();
        *e.get<TurretComponent>() = std::get<TurretComponent>(proto);
      }
      break;
    case IndexInVariant<ComponentPrototype, ItemName>:
      {
        e.add<TileItemComponent>();
        RefT<TileItemComponent> tileItem = e.get<TileItemComponent>();
        Entity itemEntity = loader.getPrefab(std::get<ItemName>(proto).item);
        tileItem->item = itemEntity;
        itemEntity.add<ItemAttrTile>();
        RefT<ItemAttrTile> tile = itemEntity.get<ItemAttrTile>();
        if (id == 0) {
          throw std::runtime_error("Items cannot be created within a scene (AssetId was 0)");
        }

        tile->tileId = id;
      }
      break;

    // If an entity prototype contains any one of the components below
    // the other two item components must be added, it is necessary
    // for the item system to work.
    case IndexInVariant<ComponentPrototype, ItemAttrStackCost>:
      {
        e.add<ItemAttrIcon>();
        e.add<ItemAttrStackCost>();
        *e.get<ItemAttrStackCost>() = std::get<ItemAttrStackCost>(proto);
      }
      break;
    case IndexInVariant<ComponentPrototype, ItemIconPath>:
      {
        e.add<ItemAttrIcon>();
        e.add<ItemAttrStackCost>();
        RefT<ItemAttrIcon> icon = e.get<ItemAttrIcon>();
        icon->icon = tgui::Texture(path + std::get<ItemIconPath>(proto).icon);
      }
      break;
    case IndexInVariant<ComponentPrototype, ItemAttrUnique>:
      {
        e.add<ItemAttrIcon>();
        e.add<ItemAttrStackCost>();
        e.add<ItemAttrUnique>();
      }
      break;
      // end of comment

    case IndexInVariant<ComponentPrototype, TileBreakable>:
      {
        e.add<HealthComponent>();
        RefT<HealthComponent> health = e.get<HealthComponent>();

        e.add<ArrowComponent>();
        e.get<ArrowComponent>()->tileCost = static_cast<u32>(health->health);
        e.add<TileBoundComponent>();
        e.add<DestroyTileOnEntityRemovalTag>();
      }
      break;
    case IndexInVariant<ComponentPrototype, TransformPrototype>:
      {
        e.add<TransformComponent>();
        *e.get<TransformComponent>() = std::get<TransformPrototype>(proto).toTransform();
      }
      break;
    case IndexInVariant<ComponentPrototype, SpriteIndependentTag>:
      {
        e.add<SpriteIndependentTag>();
      }
      break;
    case IndexInVariant<ComponentPrototype, AddBodyComponent>:
      {
        e.add<AddBodyComponent>();
        *e.get<AddBodyComponent>() = std::get<AddBodyComponent>(proto);
      }
      break;
    default:
      break;
    }
  }

  return AssetLoader::PrefabAsset(e);
}

TileDef loadTilePrototype(AssetLoader& loader, EntityWorld& world, const std::string& path, AssetId id,
                          const TilePrototype& tilePrototype) {
  TileDef def = tilePrototype.tile;
  if (!tilePrototype.entityComponents.comps.empty()) {
    def.entity = loadPrefabPrototype(loader, world, path, id, tilePrototype.entityComponents).entity;
    Entity e = world.get(def.entity);

    e.add<TransformComponent>();
    e.add<TileBoundComponent>();
    e.add<DestroyTileOnEntityRemovalTag>();

    if (!def.enableCollision) {
      e.add<ArrowComponent>();
    }

    for (const ComponentPrototype& proto : tilePrototype.entityComponents.comps) {
      if (proto.index() == IndexInVariant<ComponentPrototype, TileBreakable>) {
        e.add<HealthComponent>();
        RefT<HealthComponent> health = e.get<HealthComponent>();
        health->health = std::get<TileBreakable>(proto).health;

        e.add<ArrowComponent>();
        e.get<ArrowComponent>()->tileCost = static_cast<u32>(health->health);

        def.shapeDef.enableHitEvents = true;
        def.shapeDef.enableContactEvents = true;
        break;
      }
    }
  }

  return def;
}

Scene loadScenePrototype(AssetLoader& loader, EntityWorld& world, const std::string& path,
                         const ScenePrototype& proto) {
  Scene scene(world, proto.name, proto.desc);

  // TODO: logGeneric("<bgYellow, fgWhite>Creating scene %s\n<reset>", proto.name.c_str());
  for (const std::string& assetFile : proto.load) {
    if (!loader.loadAssets(path + assetFile)) {
      // TODO: logGeneric("\t<bgRed, fgWhite>Failed load! %s\n<reset>", assetFile.c_str());
    } else {
      // TODO: logGeneric("\tLoaded for scene: %s\n", assetFile.c_str());
    }
  }

  for (const SceneAssetPrototype& proto : proto.scene) {
    EntityId overrideEntity = NullEntityId;
    if (!proto.override.comps.empty())
      overrideEntity = loadPrefabPrototype(loader, world, path, 0, proto.override).entity;

    scene.addPrefab(loader.getPrefab(proto.basePrefab), overrideEntity);
  }

  // TODO: logGeneric("<bgGreen, fgWhite>Scene created.<reset>\n", proto.name.c_str());

  return scene;
}

bool AssetLoader::loadAssets(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open())
    return false;

  std::string fileData;
  fileData.resize(std::filesystem::file_size(path));
  file.read(fileData.data(), fileData.size());
  file.close();

  constexpr glz::opts readOps{
      .comments = true,
      .error_on_unknown_keys = true,
      .append_arrays = true,
      .error_on_missing_keys = false // turn this to true for debug reasons
  };

  std::string parentDir = getPath(path);
  std::vector<AssetJson> readAssets;
  glz::error_ctx ec = glz::read<readOps>(readAssets, fileData);
  if (ec) {
    std::string msg = glz::format_error(ec, fileData);
    // TODO: logGeneric("<bgRed, bold>Failed to load assets\n Ec: %i\n File:%s\n Msg: "%s\n<reset>", ec.ec, path.c_str(), msg.c_str());
    return false;
  }

  for (const AssetJson& json : readAssets) {
    // TODO: logGeneric("<fgGreen>Loading Asset: %s\n<reset>", json.name.c_str());
    loadAsset(parentDir, json);
  }

  return true;
}

AssetId AssetLoader::loadAsset(const std::string& parentDir, const AssetJson& asset) {
  switch (asset.prototype.index()) {
  case IndexInVariant<AssetPrototype, SpritePrototype>:
    return createAsset(asset.name,
                       loadSprite(m_world, parentDir, std::get<SpritePrototype>(asset.prototype)));
  case IndexInVariant<AssetPrototype, TilePrototype>:
    {
      AssetId assetId = createAsset<TileDef>(asset.name);
      getTilePrefab(assetId) =
          loadTilePrototype(*this, m_world, parentDir, assetId, std::get<TilePrototype>(asset.prototype));
      return assetId;
    }
  case IndexInVariant<AssetPrototype, PrefabPrototype>:
    {
      AssetId assetId = createAsset<PrefabAsset>(asset.name);
      getPrefabRawId(assetId) =
          loadPrefabPrototype(*this, m_world, parentDir, assetId, std::get<PrefabPrototype>(asset.prototype))
              .entity;
      return assetId;
    }
  case IndexInVariant<AssetPrototype, ScenePrototype>:
    {
      AssetId assetId = createAsset<SceneAsset>(asset.name);
      std::get<SceneAsset>(m_assets.find(assetId)->second).scene = std::make_shared<Scene>(
          loadScenePrototype(*this, m_world, parentDir, std::get<ScenePrototype>(asset.prototype)));
      return assetId;
    }
  default:
    // TODO: logGeneric("Failed to load asset %s\n", asset.name.c_str());
    return 0;
  }
}

RAMPAGE_END