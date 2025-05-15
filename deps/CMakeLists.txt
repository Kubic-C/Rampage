set(BUILD_SHARED_LIBS OFF)

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
set(TGUI_BUILD_GUI_BUILDER OFF)
add_subdirectory("SDL")
set(SDLTTF_VENDORED ON)
add_subdirectory("SDL_ttf")
add_subdirectory("TGUI")

add_subdirectory("glaze")