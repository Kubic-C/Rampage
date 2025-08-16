
glad_add_library(GLAD STATIC REPRODUCIBLE API gl:core=4.0)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Master DLL
# Think of this as what all other DLLs will link against for symbols.

add_library(RMaster SHARED
  "app.hpp"
  "assetLoader.cpp"
  "assetLoader.hpp"
  "contexts.hpp"
  "eventManager.hpp"
  "inventory.cpp"
  "inventory.hpp"
  "ecs/ref.cpp"
  "temp/utility/TaskScheduler.cpp"
  "temp/utility/TaskScheduler.hpp"
  "temp/utility/LockLessMultiReadPipe.hpp"
  "ecs/serter.cpp"
  "ecs/serter.hpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RMaster PROPERTY CXX_STANDARD 20)
  set_property(TARGET tgui PROPERTY CXX_STANDARD 20)
endif()

# Utility
target_sources(
  RMaster PUBLIC
  "temp/utility/base.hpp"
  "temp/utility/id.hpp"
  "temp/utility/math.hpp"
  "temp/utility/log.hpp"
  "temp/utility/log.cpp")

# Render
target_sources(
  RMaster PUBLIC
  "render/baseRender.hpp"
  "render/render.hpp"
  "render/camera.hpp"
  "render/stb_image.h"
  "render/stb_image.c"
  "render/util.hpp"
  "render/vertexArrayBuffer.hpp"
  "render/vertexBuffer.hpp"
  "render/shader.hpp"
  "render/textureArray.hpp"
  "render/mesh.hpp")

# State
target_sources(
  RMaster PUBLIC
  "states/menuState.hpp"
  "states/playState.hpp"
  "states/state.hpp")

# Components
target_sources(
  RMaster PUBLIC
  "components/arrow.hpp"
  "components/body.hpp"
  "components/collisionQueue.hpp"
  "components/health.hpp"
  "components/player.hpp"
  "components/sprite.hpp"
  "components/tile.hpp"
  "components/tilemap.hpp"
  "components/transform.hpp"
  "components/turret.hpp"
  "components/shapes.hpp"
  "components/worldMap.hpp")

# Components
target_sources(
  RMaster PUBLIC
  "modules/guiRender.hpp"
  "modules/health.hpp"
  "modules/pathfinding.hpp"
  "modules/physics.hpp"
  "modules/player.hpp"
  "modules/spriteRender.hpp"
  "modules/shapeRender.hpp"
  "modules/tilemap.hpp"
  "modules/turret.hpp"
  "modules/item.hpp")

# Ecs
target_sources(
  RMaster PUBLIC
  "ecs/componentSet.cpp"
  "ecs/componentSet.hpp"
  "ecs/ecs.hpp"
  "ecs/entity.cpp"
  "ecs/entity.hpp"
  "ecs/id.hpp"
  "ecs/module.hpp"
  "ecs/pool.hpp"
  "ecs/system.hpp"
  "ecs/world.hpp"
  "ecs/world.cpp")

target_sources(RMaster PUBLIC ${srcFiles})

set(NeededBoostLibs
  "boost_type_index"
  "boost_container"
  "boost_unordered"
  "boost_pool")

target_link_libraries(RMaster PUBLIC
  ${NeededBoostLibs}
  glm-header-only
  GLAD
  box2d
  TGUI::TGUI
  SDL3::SDL3
  glaze::glaze
  CapnProto::capnp)

file(GLOB schemasCXX "./schema/*.c++" "./schema/*.h")

target_sources(RMaster PUBLIC ${schemasCXX})

add_dependencies(RMaster capnpc_cpp capnp_tool)