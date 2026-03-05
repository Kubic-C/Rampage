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
    positionsBuilder[i].setZ((i32)pos.z);
  }
  
  // Serialize anchorPos
  auto anchorPosBuilder = multiTileBuilder.getAnchorPos();
  anchorPosBuilder.setX((i32)self->anchorPos.x);
  anchorPosBuilder.setY((i32)self->anchorPos.y);
  anchorPosBuilder.setZ((i32)self->anchorPos.z);
}

void MultiTileComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto multiTileReader = reader.getRoot<Schema::MultiTileComponent>();
  auto self = component.cast<MultiTileComponent>();
  
  // Deserialize occupiedPositions
  self->occupiedPositions.clear();
  const auto positionsReader = multiTileReader.getOccupiedPositions();
  for (auto posReader : positionsReader) {
    glm::ivec3 pos((i32)posReader.getX(), (i32)posReader.getY(), (i32)posReader.getZ());
    self->occupiedPositions.push_back(pos);
  }
  
  // Deserialize anchorPos
  const auto anchorPosReader = multiTileReader.getAnchorPos();
  self->anchorPos = glm::ivec3((i32)anchorPosReader.getX(), (i32)anchorPosReader.getY(), (i32)anchorPosReader.getZ());
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
    if (anchorJson.hasZ())
      self->anchorPos.z = anchorJson.getZ();
  }
  
  // Parse occupiedPositions array
  if (compJson->hasOccupiedPositions()) {
    self->occupiedPositions.clear();
    for (const auto& posJson : compJson->getOccupiedPositions()) {
      glm::ivec3 pos(0);
      if (posJson.hasX())
        pos.x = posJson.getX();
      if (posJson.hasY())
        pos.y = posJson.getY();
      if (posJson.hasZ())
        pos.z = posJson.getZ();
      self->occupiedPositions.push_back(pos);
    }
  }
}

void TileComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto tileBuilder = builder.initRoot<Schema::TileComponent>();
  auto self = component.cast<TileComponent>();
  
  // Serialize collidable
  tileBuilder.setCollidable(self->collidable);

  // Serialize pos
  auto posBuilder = tileBuilder.getPos();
  posBuilder.setX((i32)self->pos.x);
  posBuilder.setY((i32)self->pos.y);
  posBuilder.setZ((i32)self->pos.z);
  
  // Serialize parent
  tileBuilder.setParent(self->parent);
  
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

  // Deserialize pos
  const auto posReader = tileReader.getPos();
  self->pos = glm::ivec3(posReader.getX(), posReader.getY(), posReader.getZ());
  
  // Deserialize parent
  self->parent = idMapper.resolve(tileReader.getParent());
  
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
  auto tilesBuilder = tilemapBuilder.initTiles((u32)self->tiles.size());
  
  u32 i = 0;
  for (const auto& [pos, entityId] : self->tiles) {
    auto tileEntryBuilder = tilesBuilder[i++];
    auto posBuilder = tileEntryBuilder.getPos();
    posBuilder.setX((i32)pos.x);
    posBuilder.setY((i32)pos.y);
    posBuilder.setZ((i32)pos.z);
    tileEntryBuilder.setEntityId(entityId);
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
  
  // Deserialize tiles list back into map
  self->tiles.clear();
  const auto tilesReader = tilemapReader.getTiles();
  for (auto tileEntryReader : tilesReader) {
    const auto posReader = tileEntryReader.getPos();
    glm::ivec3 pos(posReader.getX(), posReader.getY(), posReader.getZ());
    EntityId entityId = idMapper.resolve(tileEntryReader.getEntityId());
    self->tiles[pos] = entityId;

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
    world->getEntity(self->tiles[TilemapComponent::getNearestTile(pos)]).get<TileComponent>()->shapeId = shapeId;
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

  for(const auto& [pos, entityId] : srcTilemap->tiles) {
    dstTilemap->tiles[pos] = world->clone(entityId);
  }

  auto body = dst.getEntity().get<BodyComponent>()->id;
  u32 shapeCount = b2Body_GetShapeCount(body);
  std::vector<b2ShapeId> shapeIds(shapeCount); // to keep track of shapeIds for cleanup if needed
  b2Body_GetShapes(body, shapeIds.data(), shapeCount);
  for (u32 i = 0; i < shapeCount; i++) {
    b2ShapeId shapeId = shapeIds[i];
    Vec2 pos = getCenterOfShape(shapeId); // Get the position of the shape's center
    world->getEntity(dstTilemap->tiles[TilemapComponent::getNearestTile(pos)]).get<TileComponent>()->shapeId = shapeId;
  }
}

RAMPAGE_END