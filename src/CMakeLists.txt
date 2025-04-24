
glad_add_library(GLAD SHARED REPRODUCIBLE API gl:core=4.0)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Add source to this project's executable.
add_executable(Rampage 
	"app.hpp" 
	"assetLoader.cpp"
	"assetLoader.hpp"
	"contexts.hpp"
	"eventManager.hpp"
	"inventory.cpp"
	"inventory.hpp"
	"main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Rampage PROPERTY CXX_STANDARD 20)
endif()

# Utility
target_sources(
	Rampage PUBLIC
	"utility/ecs.hpp"
	"utility/ecs.cpp"
	"utility/base.hpp"
	"utility/id.hpp"
	"utility/math.hpp"
	"utility/log.hpp"
	"utility/log.cpp")

# Render
target_sources(
	Rampage PUBLIC
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
	Rampage PUBLIC
	"states/menuState.hpp"
	"states/playState.hpp"
	"states/state.hpp")

# Components
target_sources(
	Rampage PUBLIC
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
	Rampage PUBLIC
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

target_sources(Rampage PUBLIC ${srcFiles})

# TODO: Add tests and install targets if needed.

# Boost
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_LIBS   ON)
set(BOOST_ROOT "$ENV{BOOST_ROOT}")
find_package(Boost REQUIRED COMPONENTS thread)
target_include_directories(Rampage PUBLIC "${Boost_INCLUDE_DIRS}")
target_link_directories(Rampage PUBLIC "${Boost_LIBRARY_DIRS}")
target_link_libraries(Rampage PUBLIC Boost::thread)

target_link_libraries(Rampage PUBLIC glm-header-only GLAD box2d tgui SDL3::SDL3 nlohmann_json)