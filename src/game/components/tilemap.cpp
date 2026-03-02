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

void MultiTileComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<MultiTileComponent>();
  
  // Parse anchorPos if provided
  if (compJson.contains("anchorPos") && compJson["anchorPos"].is_object()) {
    const auto& anchorJson = compJson["anchorPos"];
    if (anchorJson.contains("x") && anchorJson["x"].is_number_integer())
      self->anchorPos.x = anchorJson["x"];
    if (anchorJson.contains("y") && anchorJson["y"].is_number_integer())
      self->anchorPos.y = anchorJson["y"];
    if (anchorJson.contains("z") && anchorJson["z"].is_number_integer())
      self->anchorPos.z = anchorJson["z"];
  }
  
  // Parse occupiedPositions array
  if (compJson.contains("occupiedPositions") && compJson["occupiedPositions"].is_array()) {
    self->occupiedPositions.clear();
    for (const auto& posJson : compJson["occupiedPositions"]) {
      if (posJson.is_object()) {
        glm::ivec3 pos(0);
        if (posJson.contains("x") && posJson["x"].is_number_integer())
          pos.x = posJson["x"];
        if (posJson.contains("y") && posJson["y"].is_number_integer())
          pos.y = posJson["y"];
        if (posJson.contains("z") && posJson["z"].is_number_integer())
          pos.z = posJson["z"];
        self->occupiedPositions.push_back(pos);
      }
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

void TileComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<TileComponent>();
  
  // Parse collidable if provided
  if (compJson.contains("collidable") && compJson["collidable"].is_boolean()) {
    self->collidable = compJson["collidable"];
  }

  // Parse pos if provided
  if (compJson.contains("pos") && compJson["pos"].is_object()) {
    const auto& posJson = compJson["pos"];
    if (posJson.contains("x") && posJson["x"].is_number_integer())
      self->pos.x = posJson["x"];
    if (posJson.contains("y") && posJson["y"].is_number_integer())
      self->pos.y = posJson["y"];
    if (posJson.contains("z") && posJson["z"].is_number_integer())
      self->pos.z = posJson["z"];
  }
  
  // Parse material if provided
  if (compJson.contains("material") && compJson["material"].is_object()) {
    const auto& matJson = compJson["material"];
    if (matJson.contains("friction") && matJson["friction"].is_number())
      self->material.friction = matJson["friction"];
    if (matJson.contains("restitution") && matJson["restitution"].is_number())
      self->material.restitution = matJson["restitution"];
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

void TilemapComponent::deserialize(capnp::MessageReader& reader, const IdMapper& idMapper, Ref component) {
  auto tilemapReader = reader.getRoot<Schema::TilemapComponent>();
  auto self = component.cast<TilemapComponent>();
  
  // Deserialize tiles list back into map
  self->tiles.clear();
  const auto tilesReader = tilemapReader.getTiles();
  for (auto tileEntryReader : tilesReader) {
    const auto posReader = tileEntryReader.getPos();
    glm::ivec3 pos(posReader.getX(), posReader.getY(), posReader.getZ());
    EntityId entityId = tileEntryReader.getEntityId();
    self->tiles[pos] = idMapper.resolve(entityId);
  }
}

void TilemapComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<TilemapComponent>();
  
  // Parse tiles map if provided
  if (compJson.contains("tiles") && compJson["tiles"].is_array()) {
    self->tiles.clear();
    for (const auto& tileJson : compJson["tiles"]) {
      if (tileJson.is_object()) {
        glm::ivec3 pos(0);
        EntityId entityId = 0;
        
        if (tileJson.contains("pos") && tileJson["pos"].is_object()) {
          const auto& posJson = tileJson["pos"];
          if (posJson.contains("x") && posJson["x"].is_number_integer())
            pos.x = posJson["x"];
          if (posJson.contains("y") && posJson["y"].is_number_integer())
            pos.y = posJson["y"];
          if (posJson.contains("z") && posJson["z"].is_number_integer())
            pos.z = posJson["z"];
        }
        
        if (tileJson.contains("entityId") && tileJson["entityId"].is_string()) {
          entityId = loader.cloneAsset(tileJson["entityId"]);
        }
        
        if (entityId != 0) {
          self->tiles[pos] = entityId;
        }
      }
    }
  }
}

RAMPAGE_END