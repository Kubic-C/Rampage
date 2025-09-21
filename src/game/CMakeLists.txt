add_library(RGame STATIC "module.hpp" "module.cpp")
set_property(TARGET RGame PROPERTY CXX_STANDARD 20)

target_link_libraries(RGame PUBLIC RCommon TGUI::TGUI)

target_sources(
  RGame PUBLIC
  "components/arrow.hpp"
  "components/body.hpp"
  "components/collisionQueue.hpp"
  "components/health.hpp"
  "components/player.hpp"
  "components/sprite.hpp"
  "components/tile.hpp"
  "components/tilemap.hpp"
  "components/turret.hpp"
  "components/shapes.hpp"
  "components/worldMap.hpp"
  "components/components.hpp")

target_sources(
  RGame PUBLIC
  "states/menuState.hpp"
  "states/playState.hpp"
  "states/state.hpp")

target_sources(
  RGame PUBLIC
  "assetLoader.cpp"
  "assetLoader.hpp"
  "inventory.cpp"
  "inventory.hpp"
  "scene.hpp")