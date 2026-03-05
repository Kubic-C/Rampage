#include "sprite.hpp"
#include "../../render/render.hpp"

RAMPAGE_START

// SpriteComponent
void SpriteComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
  auto spriteBuilder = builder.initRoot<Schema::SpriteComponent>();
  auto sprite = component.cast<SpriteComponent>();

  spriteBuilder.setScaling(sprite->scaling);

  // Flatten 2D subSprites into list
  auto listBuilder = spriteBuilder.initSubSprites(
      std::accumulate(sprite->subSprites.begin(), sprite->subSprites.end(), size_t(0),
                      [](size_t sum, const auto& row){ return sum + row.size(); }));

  u32 i = 0;
  u16 y = 0;
  for (const auto& row : sprite->subSprites) {
    u16 x = 0;  // Reset x for each new row
    for (const auto& sub : row) {
      auto subBuilder = listBuilder[i++];
      subBuilder.getGridPos().setX(x);
      subBuilder.getGridPos().setY(y);

      auto layersBuilder = subBuilder.initLayers(sub.layerCount);
      for (u32 j = 0; j < sub.layerCount; j++) {
        auto& layer = sub.layers[j];
        auto layerBuilder = layersBuilder[j];
        layerBuilder.setTexIndex(layer.texIndex);
        layerBuilder.getOffset().setX(layer.offset.x);
        layerBuilder.getOffset().setY(layer.offset.y);
        layerBuilder.setRot(layer.rot);
        layerBuilder.setLayer(static_cast<u8>(layer.layer));
      }

      x++;
    }
    y++;
  }
}

void SpriteComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
  auto spriteReader = reader.getRoot<Schema::SpriteComponent>();
  auto sprite = component.cast<SpriteComponent>();

  sprite->scaling = spriteReader.getScaling();
  sprite->subSprites.clear();

  const auto subList = spriteReader.getSubSprites();
  for (auto subReader : subList) {
    u16 row = subReader.getGridPos().getY();
    u16 col = subReader.getGridPos().getX();

    // Make sure your 2D vector has enough rows
    if (sprite->subSprites.size() <= row)
        sprite->subSprites.resize(row + 1);
    if (sprite->subSprites[row].size() <= col)
        sprite->subSprites[row].resize(col + 1);

    // Insert the SubSprite into the correct position
    SubSprite& sub = sprite->subSprites[row][col];

    const auto layersList = subReader.getLayers();
    sub.layerCount = layersList.size();
    for (u32 j = 0; j < layersList.size(); j++) {
      auto layerReader = layersList[j];
      auto& layer = sub.layers[j];  // Use sequential index, not enum value
      layer.texIndex = layerReader.getTexIndex();
      layer.offset.x = layerReader.getOffset().getX();
      layer.offset.y = layerReader.getOffset().getY();
      layer.rot = layerReader.getRot();
      layer.layer = static_cast<WorldLayer>(layerReader.getLayer());
    }
  }
}

void SpriteComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
  auto self = component.cast<SpriteComponent>();
  IWorldPtr world = self.getWorld();
  auto compJson = jsonValue.as<JSchema::SpriteComponent>();
  
  // Parse scaling if provided
  if (compJson->hasScaling()) {
    self->scaling = compJson->getScaling();
  }
  
  // Parse subSprites array
  if (compJson->hasSubSprites()) {
    const auto& subSpritesJson = compJson->getSubSprites();
    
    for (const auto& subSpriteJson : subSpritesJson) {
      // Parse grid position if provided, otherwise default to (0, 0)
      u16 gridX = 0;
      u16 gridY = 0;
      
      if (subSpriteJson.hasGridPos()) {
        auto gridPosJson = subSpriteJson.getGridPos();
        if (gridPosJson.hasX())
          gridX = gridPosJson.getX();
        if (gridPosJson.hasY())
          gridY = gridPosJson.getY();
      }
      
      SubSprite subSprite;
      
      // Parse layers array for this subSprite
      if (subSpriteJson.hasLayers()) {
        const auto& layersJson = subSpriteJson.getLayers();
        
        for (const auto& layerJson : layersJson) {
          u32 texIndex = 0;
          glm::vec2 offset = Vec2(0);
          float rot = 0.0f;
          WorldLayer layer = WorldLayer::Invalid;
          
          // Load texture by path (get index from TextureMapInUse)
          if (layerJson.hasPath()) {
            const std::string& texturePath = layerJson.getPath();
            EntityPtr texMapEntity = world->getFirstWith({world->component<TextureMapInUseTag>()});
            auto texMap = texMapEntity.get<TextureMap3DComponent>();
            texIndex = texMap->getSprite(texturePath);
          }
          
          // Parse offset if provided
          if (layerJson.hasOffset()) {
            auto offsetJson = layerJson.getOffset();
            if (offsetJson.hasX())
              offset.x = offsetJson.getX();
            if (offsetJson.hasY())
              offset.y = offsetJson.getY();
          }
          
          // Parse rotation if provided
          if (layerJson.hasRot()) {
            rot = layerJson.getRot();
          }
          
          // Parse layer name (string to enum conversion)
          if (layerJson.hasLayer()) {
            const std::string& layerName = layerJson.getLayer();
            if (layerName == "Bottom") layer = WorldLayer::Bottom;
            else if (layerName == "Floor") layer = WorldLayer::Floor;
            else if (layerName == "Wall") layer = WorldLayer::Wall;
            else if (layerName == "Turret") layer = WorldLayer::Turret;
            else if (layerName == "Item") layer = WorldLayer::Item;
            else if (layerName == "Top") layer = WorldLayer::Top;
            else layer = WorldLayer::Invalid;
          }
          
          subSprite.addLayer(texIndex, offset, rot, layer);
        }
      }
      
      // Add the subSprite to the component's 2D vector at the correct grid position
      if (subSprite.layerCount > 0) {
        // Resize to accommodate the grid position
        if (self->subSprites.size() <= gridY) {
          self->subSprites.resize(gridY + 1);
        }
        if (self->subSprites[gridY].size() <= gridX) {
          self->subSprites[gridY].resize(gridX + 1);
        }
        
        self->subSprites[gridY][gridX] = subSprite;
      }
    }
  }
}

RAMPAGE_END