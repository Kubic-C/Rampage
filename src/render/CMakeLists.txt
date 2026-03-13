add_library(RRender STATIC "module.hpp" "module.cpp")
set_property(TARGET RRender PROPERTY CXX_STANDARD 20)

target_sources(RRender PUBLIC
  "render2D/stb_image.h"
  "render2D/stb_image.c"
  "render2D/render2D.hpp"
  "render2D/render2D.cpp"
  "render2D/internal.hpp"
  "render2D/internal.cpp"
  "render2D/common.hpp"
  "render2D/commands.hpp"
  "camera.hpp"
  "camera.cpp")

target_link_libraries(RRender PUBLIC RCommon bx)