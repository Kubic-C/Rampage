add_library(RGame STATIC "module.hpp" "module.cpp")
set_property(TARGET RGame PROPERTY CXX_STANDARD 20)

target_link_libraries(RGame PUBLIC RCommon TGUI::TGUI)

target_sources(
  RGame PUBLIC
  "components/arrow.hpp"
  "components/body.hpp"
  "components/body.cpp"
  "components/collisionEvent.hpp"
  "components/health.hpp"
  "components/health.cpp"
  "components/player.cpp"
  "components/player.hpp"
  "components/spawner.hpp"
  "components/spawner.cpp"
  "components/sprite.hpp"
  "components/sprite.cpp"
  "components/tilemap.cpp"
  "components/tilemap.hpp"
  "components/turret.hpp"
  "components/turret.cpp"   
  "components/shapes.hpp"  
  "components/shapes.cpp"  
  "components/items.hpp"
  "components/items.cpp"
  "components/inventory.hpp"
  "components/inventory.cpp"
  "components/worldMap.hpp"
  "components/ownedBy.hpp"
  "components/ownedBy.cpp"
  "components/components.hpp")

target_sources(
  RGame PUBLIC
  "states/menuState.hpp"
  "states/playState.hpp"
  "states/lostState.hpp"
  "states/state.hpp")

target_sources(
  RGame PUBLIC
  "scene.hpp")

target_sources(
  RGame PUBLIC
  "systems/spriteRender.cpp"
  "systems/spriteRender.hpp"
  "systems/tilemap.cpp"
  "systems/tilemap.hpp"
  "systems/inventory.hpp"
  "systems/inventory.cpp"
  "systems/spawner.cpp"
  "systems/spawner.hpp"
  "systems/player.cpp"
  "systems/player.hpp"
  "systems/physics.cpp"
  "systems/physics.hpp"
  "systems/pathfinding.cpp"
  "systems/pathfinding.hpp"
  "systems/health.cpp"
  "systems/health.hpp"
  "systems/gui.cpp"
  "systems/gui.hpp"
  "systems/turret.cpp"
  "systems/turret.hpp"
  "systems/shapeRender.cpp"
  "systems/shapeRender.hpp")