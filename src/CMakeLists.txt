
glad_add_library(GLAD SHARED REPRODUCIBLE API gl:core=4.0)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Add source to this project's executable.
add_executable (Rampage "main.cpp" "utility/log.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Rampage PROPERTY CXX_STANDARD 20)
endif()

target_sources(
	Rampage PUBLIC
	"utility/ecs.hpp"
	"utility/ecs.cpp"
	"utility/base.hpp"
	"utility/id.hpp"
	"utility/math.hpp"
	"utility/log.hpp"
	"utility/log.cpp"
	"render/render.hpp"
	"render/camera.hpp"
	"render/stb_image.h"
	"render/stb_image.c"
	"transform.hpp"
	"app.hpp"
	"tile.hpp"
	"tilemap.hpp"
	"physics.hpp"
	"seekPlayer.hpp"
	"worldMap.hpp")

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

target_link_libraries(Rampage PUBLIC glm-header-only GLAD box2d tgui SDL3::SDL3)