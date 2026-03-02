add_library(RRender STATIC "module.hpp" "module.cpp")
set_property(TARGET RRender PROPERTY CXX_STANDARD 20)

target_sources(RRender PUBLIC
  "camera.hpp"
  "camera.cpp"
  "stb_image.h"
  "stb_image.c"
  "opengl/util.cpp"
  "opengl/util.hpp"
  "opengl/vertexArrayBuffer.hpp"
  "opengl/vertexBuffer.hpp"
  "opengl/shader.hpp"
  "opengl/textureArray.hpp"
  "opengl/mesh.hpp")

target_link_libraries(RRender PUBLIC RCommon)