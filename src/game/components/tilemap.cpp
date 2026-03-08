#include "tilemap.hpp"

RAMPAGE_START

void MultiTileComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto multiTileBuilder = builder.initRoot<Schema::MultiTileComponent>();
  auto self = component.cast<MultiTileComponent>();
  
  // Serialize occupiedPositions
  auto positionsBuilder = multiTileBuilder.initOccupiedPositions((u32)self->occupiedPositions.size());
  for (size_t i = 0; i < self->occupiedPositions.size(); ++i) {
    const auto& pos = self->occupiedPositions[i];
    positionsBuilder[i].setX((i32)pos.x);
    positionsBuilder[i].setY((i32)pos.y);
    positionsBuilder[i].setZ(0);
  }
  
  // Serialize anchorPos
  auto anchorPosBuilder = multiTileBuilder.getAnchorPos();
  anchorPosBuilder.setX((i32)self->anchorPos.x);
  anchorPosBuilder.setY((i32)self->anchorPos.y);
  anchorPosBuilder.setZ(0);
}

void MultiTileComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto multiTileReader = reader.getRoot<Schema::MultiTileComponent>();
  auto self = component.cast<MultiTileComponent>();
  
  // Deserialize occupiedPositions
  self->occupiedPositions.clear();
  const auto positionsReader = multiTileReader.getOccupiedPositions();
  for (auto posReader : positionsReader) {
    glm::ivec2 pos((i32)posReader.getX(), (i32)posReader.getY());
    self->occupiedPositions.push_back(pos);
  }
  
  // Deserialize anchorPos
  const auto anchorPosReader = multiTileReader.getAnchorPos();
  self->anchorPos = glm::ivec2((i32)anchorPosReader.getX(), (i32)anchorPosReader.getY());
}

void MultiTileComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<MultiTileComponent>();
  auto compJson = jsonValue.as<JSchema::MultiTileComponent>();
  
  // Parse anchorPos if provided
  if (compJson->hasAnchorPos()) {
    auto anchorJson = compJson->getAnchorPos();
    if (anchorJson.hasX())
      self->anchorPos.x = anchorJson.getX();
    if (anchorJson.hasY())
      self->anchorPos.y = anchorJson.getY();
  }
  
  // Parse occupiedPositions array
  if (compJson->hasOccupiedPositions()) {
    self->occupiedPositions.clear();
    for (const auto& posJson : compJson->getOccupiedPositions()) {
      glm::ivec2 pos(0);
      if (posJson.hasX())
        pos.x = posJson.getX();
      if (posJson.hasY())
        pos.y = posJson.getY();
      self->occupiedPositions.push_back(pos);
    }
  }
}

void TileComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tileBuilder = builder.initRoot<Schema::TileComponent>();
  auto self = component.cast<TileComponent>();
  
  // Serialize collidable
  tileBuilder.setCollidable(self->collidable);

  // Serialize pos and layer
  auto posBuilder = tileBuilder.getPos();
  posBuilder.setX((i32)self->pos.x);
  posBuilder.setY((i32)self->pos.y);
  posBuilder.setZ((i32)self->layer);
  
  // Serialize parent
  tileBuilder.setParent(self->parent);
  
  // Serialize rotation
  tileBuilder.setRotation(static_cast<u8>(self->rotation));

  // Serialize material
  auto matBuilder = tileBuilder.getMaterial();
  matBuilder.setFriction(self->material.friction);
  matBuilder.setRestitution(self->material.restitution);
  matBuilder.setRollingResistance(self->material.rollingResistance);
  matBuilder.setTangentSpeed(self->material.tangentSpeed);
  matBuilder.setCustomColor(self->material.customColor);
}

void TileComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto tileReader = reader.getRoot<Schema::TileComponent>();
  auto self = component.cast<TileComponent>();
  
  // Deserialize collidable
  self->collidable = tileReader.getCollidable();

  // Deserialize pos and layer
  const auto posReader = tileReader.getPos();
  self->pos = glm::ivec2(posReader.getX(), posReader.getY());
  self->layer = static_cast<WorldLayer>(posReader.getZ());
  
  // Deserialize parent
  self->parent = idMapper.resolve(tileReader.getParent());

  // Deserialize rotation
  self->rotation = static_cast<TileDirection>(tileReader.getRotation());
  
  // Deserialize material
  const auto matReader = tileReader.getMaterial();
  self->material.friction = matReader.getFriction();
  self->material.restitution = matReader.getRestitution();
  self->material.rollingResistance = matReader.getRollingResistance();
  self->material.tangentSpeed = matReader.getTangentSpeed();
  self->material.userMaterialId = matReader.getUserMaterialId();
  self->material.customColor = matReader.getCustomColor();
}

void TileComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<TileComponent>();
  auto compJson = jsonValue.as<JSchema::TileComponent>();
  
  // Parse collidable if provided
  if (compJson->hasCollidable()) {
    self->collidable = compJson->getCollidable();
  }

  // Parse pos if provided
  if (compJson->hasPos()) {
    auto posJson = compJson->getPos();
    if (posJson.hasX())
      self->pos.x = posJson.getX();
    if (posJson.hasY())
      self->pos.y = posJson.getY();
  }

  // Parse layer if provided (string to enum conversion)
  if (compJson->hasLayer()) {
    const std::string& layerName = compJson->getLayer();
    if (layerName == "Floor") self->layer = WorldLayer::Floor;
    else if (layerName == "Wall") self->layer = WorldLayer::Wall;
    else if (layerName == "Item") self->layer = WorldLayer::Item;
    else if (layerName == "Top") self->layer = WorldLayer::Top;
    else self->layer = WorldLayer::Floor;
  }
  
  // Parse rotation if provided
  if (compJson->hasRotation()) {
    const std::string& rotName = compJson->getRotation();
    if (rotName == "Up") self->rotation = TileDirection::Up;
    else if (rotName == "Right") self->rotation = TileDirection::Right;
    else if (rotName == "Down") self->rotation = TileDirection::Down;
    else if (rotName == "Left") self->rotation = TileDirection::Left;
  }

  // Parse material if provided
  if (compJson->hasMaterial()) {
    auto matJson = compJson->getMaterial();
    if (matJson.hasFriction())
      self->material.friction = matJson.getFriction();
    if (matJson.hasRestitution())
      self->material.restitution = matJson.getRestitution();
  }
}

void TilemapComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tilemapBuilder = builder.initRoot<Schema::TilemapComponent>();
  auto self = component.cast<TilemapComponent>();
  
  // Serialize tiles map as list of TileEntry key-value pairs
  auto tilesBuilder = tilemapBuilder.initTiles((u32)self->size());
  
  u32 i = 0;
  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    WorldLayer layer = static_cast<WorldLayer>(l);
    for (const auto& [pos, entityId] : self->getLayer(layer)) {
      auto tileEntryBuilder = tilesBuilder[i++];
      auto posBuilder = tileEntryBuilder.getPos();
      posBuilder.setX((i32)pos.x);
      posBuilder.setY((i32)pos.y);
      posBuilder.setZ((i32)l);
      tileEntryBuilder.setEntityId(entityId);
    }
  }
}

Vec2 getCenterOfShape(b2ShapeId shapeId) {
  switch (b2Shape_GetType(shapeId))
  {
  case b2_circleShape: {
    b2Circle circle = b2Shape_GetCircle(shapeId);
    return Vec2(circle.center);
  }
  case b2_polygonShape: {
    b2Polygon poly = b2Shape_GetPolygon(shapeId);
    return Vec2(poly.centroid);
  }
  default:
    return Vec2(0.0f);
  }
}

void TilemapComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto tilemapReader = reader.getRoot<Schema::TilemapComponent>();
  auto world = component.getWorld();
  auto self = component.cast<TilemapComponent>();
  auto body = self.getEntity().get<BodyComponent>()->id;
  
  // Clear all layers
  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l)
    self->getLayer(static_cast<WorldLayer>(l)).clear();

  // Deserialize tiles list back into layered maps
  const auto tilesReader = tilemapReader.getTiles();
  for (auto tileEntryReader : tilesReader) {
    const auto posReader = tileEntryReader.getPos();
    glm::ivec2 pos(posReader.getX(), posReader.getY());
    WorldLayer layer = static_cast<WorldLayer>(posReader.getZ());
    EntityId entityId = idMapper.resolve(tileEntryReader.getEntityId());
    self->setTile(layer, pos, entityId);

    // If the entity has not yet already been deserialized, ensure its existence and add a TileComponent.
    // This will have no effect on the entity if it has not already been deserialized as
    // additional add operations are redundant and the the resolved entityId is already reserved.
    world->ensure(entityId).add<TileComponent>();
  }

  u32 shapeCount = b2Body_GetShapeCount(body);
  std::vector<b2ShapeId> shapeIds(shapeCount); // to keep track of shapeIds for cleanup if needed
  b2Body_GetShapes(body, shapeIds.data(), shapeCount);
  for (u32 i = 0; i < shapeCount; i++) {
    b2ShapeId shapeId = shapeIds[i];
    Vec2 pos = getCenterOfShape(shapeId); // Get the position of the shape's center
    glm::ivec2 tilePos = getNearestTile(pos);
    for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
      WorldLayer layer = static_cast<WorldLayer>(l);
      if (!self->containsTile(layer, tilePos)) continue;
      auto tileComp = world->getEntity(self->getTile(layer, tilePos)).get<TileComponent>();
      if (!b2Shape_IsValid(tileComp->shapeId)) {
        tileComp->shapeId = shapeId;
        b2Shape_SetUserData(shapeId, entityToB2Data(self->getTile(layer, tilePos))); // Associate shape with tilemap entity for collision handling
        break;
      }
    }
  }
}

void TilemapComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& compJson) {
  // TilemapComponent is not part of the JSON schema variant;
  // tiles are populated via deserialization, not fromJson.
}

void TilemapComponent::copy(Ref src, Ref dst) {
  auto world = src.getWorld();
  auto srcTilemap = src.cast<TilemapComponent>();
  auto dstTilemap = dst.cast<TilemapComponent>();

  for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
    WorldLayer layer = static_cast<WorldLayer>(l);
    for (const auto& [pos, entityId] : srcTilemap->getLayer(layer)) {
      dstTilemap->setTile(layer, pos, world->clone(entityId));
    }
  }

  auto body = dst.getEntity().get<BodyComponent>()->id;
  u32 shapeCount = b2Body_GetShapeCount(body);
  std::vector<b2ShapeId> shapeIds(shapeCount); // to keep track of shapeIds for cleanup if needed
  b2Body_GetShapes(body, shapeIds.data(), shapeCount);
  for (u32 i = 0; i < shapeCount; i++) {
    b2ShapeId shapeId = shapeIds[i];
    Vec2 pos = getCenterOfShape(shapeId); // Get the position of the shape's center
    glm::ivec2 tilePos = getNearestTile(pos);
    for (size_t l = 0; l < TilemapComponent::LayerCount; ++l) {
      WorldLayer layer = static_cast<WorldLayer>(l);
      if (!dstTilemap->containsTile(layer, tilePos)) continue;
      auto tileComp = world->getEntity(dstTilemap->getTile(layer, tilePos)).get<TileComponent>();
      if (!b2Shape_IsValid(tileComp->shapeId)) {
        tileComp->shapeId = shapeId;
        b2Shape_SetUserData(shapeId, entityToB2Data(dstTilemap->getTile(layer, tilePos))); // Associate shape with tilemap entity for collision handling
        break;
      }
    }
  }
}

RAMPAGE_END