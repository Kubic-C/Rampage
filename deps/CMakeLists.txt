set(BUILD_SHARED_LIBS OFF)

add_subdirectory("capnproto")

# Boost dependencies
add_subdirectory("boost/assert")
add_subdirectory("boost/static_assert")
add_subdirectory("boost/config")
add_subdirectory("boost/intrusive")
add_subdirectory("boost/move")
add_subdirectory("boost/throw_exception")
add_subdirectory("boost/type_traits")
add_subdirectory("boost/winapi")
add_subdirectory("boost/predef")
add_subdirectory("boost/mp11")
add_subdirectory("boost/container_hash")
add_subdirectory("boost/describe")
add_subdirectory("boost/core")
add_subdirectory("boost/integer")

# Needed Boost libraries
add_subdirectory("boost/container")
add_subdirectory("boost/pool")
add_subdirectory("boost/type_index")
add_subdirectory("boost/unordered")

if(ASAN_ON)
  if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BOX2D_SANITIZE ON)
  endif()
endif()

set(BOX2D_SAMPLES OFF)
set(BOX2D_UNIT_TESTS OFF)
add_subdirectory("box2d")
add_subdirectory("glad/cmake")
add_subdirectory("glm")

set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
# keep the below in order
set(TGUI_SHARED_LIBS OFF)
set(TGUI_INSTALL OFF)
set(TGUI_STATIC true)
set(TGUI_BACKEND "SDL_TTF_OPENGL3")
set(TGUI_BUILD_GUI_BUILDER ON)
add_subdirectory("SDL")
set(SDLTTF_VENDORED ON)
add_subdirectory("SDL_ttf")
add_subdirectory("TGUI")

add_subdirectory("json")