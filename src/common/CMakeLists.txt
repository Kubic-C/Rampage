glad_add_library(GLAD STATIC REPRODUCIBLE API gl:core=4.0)

add_library(RCommon STATIC
  "empty.cpp")

set_property(TARGET RCommon PROPERTY CXX_STANDARD 20)

set(NeededBoostLibs
  "boost_type_index"
  "boost_container"
  "boost_unordered"
  "boost_pool")
target_link_libraries(RCommon PUBLIC
  glaze::glaze
  ${NeededBoostLibs}
  glm::glm
  CapnProto::capnp
  box2d::box2d
  SDL3::SDL3
  GLAD)

# ECS
target_sources(
  RCommon PUBLIC
  "ecs/componentSet.cpp"
  "ecs/componentSet.hpp"
  "ecs/ecs.hpp"
  "ecs/entity.cpp"
  "ecs/entity.hpp"
  "ecs/id.hpp"
  "ecs/staticId.hpp"
  "ecs/pool.hpp"
  "ecs/ref.cpp"
  "ecs/ref.hpp"
  "ecs/serter.cpp"
  "ecs/serter.hpp"
  "ecs/system.hpp"
  "ecs/world.hpp"
  "ecs/world.cpp")

target_sources(
  RCommon PUBLIC
  "commondef.hpp"
  "math.hpp"
  "imodule.hpp"
  "ihost.hpp"
  "pipeline.cpp"
  "pipeline.hpp")

file(GLOB schemasCXX "./schema/*.c++" "./schema/*.h")

target_sources(RCommon PUBLIC ${schemasCXX})