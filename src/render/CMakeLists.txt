add_library(RRender STATIC "module.hpp" "module.cpp")
set_property(TARGET RRender PROPERTY CXX_STANDARD 20)

target_sources(RRender PUBLIC
  "camera.hpp"
  "camera.cpp"
  "stb_image.h"
  "stb_image.c"
  "render.hpp")

target_link_libraries(RRender PUBLIC RCommon)