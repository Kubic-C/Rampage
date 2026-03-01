#include "tile.hpp"

RAMPAGE_START

// TileComponent
void TileComponent::fromJson(Ref component, AssetLoader loader, const json& compJson) {
  auto self = component.cast<TileComponent>();

  if (compJson.contains("flags") && compJson["flags"].is_array()) {
    for (const auto& flagJson : compJson["flags"]) {
      if (flagJson.is_string()) {
        TileFlags flag = getTileFlagFromString(flagJson);
        if (flag != (TileFlags)std::numeric_limits<u8>::max()) {
          self->flags |= flag;
        }
      }
    }
  }

  if (compJson.contains("size") && compJson["size"].is_object()) {
    json sizeJson = compJson["size"];
    if (sizeJson.contains("x") && sizeJson["x"].is_number_unsigned())
      self->x.w = sizeJson["x"];
    if (sizeJson.contains("y") && sizeJson["y"].is_number_unsigned())
      self->y.h = sizeJson["y"];
  }
}


RAMPAGE_END
