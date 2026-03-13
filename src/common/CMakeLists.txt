add_library(RCommon STATIC
  "empty.cpp")

set_property(TARGET RCommon PROPERTY CXX_STANDARD 20)

set(NeededBoostLibs
  "boost_type_index"
  "boost_container"
  "boost_unordered"
  "boost_pool")
target_link_libraries(RCommon PUBLIC
  nlohmann_json
  ${NeededBoostLibs}
  glm::glm
  CapnProto::capnp
  box2d::box2d
  SDL3::SDL3
  box2d::box2d
  bgfx)

# ECS
target_sources(
  RCommon PUBLIC
  "ecs/componentSet.cpp"
  "ecs/componentSet.hpp"
  "ecs/ecs.hpp"
  "ecs/entityPtr.hpp"
  "ecs/id.hpp"
  "ecs/ipool.hpp"
  "ecs/ref.cpp"
  "ecs/ref.hpp"
  "ecs/serializer.hpp"
  "ecs/serializer.cpp"
  "ecs/world.hpp"
  "ecs/world.cpp"
  "ecs/iworld.hpp"
  "ecs/iworld.cpp"
  "ecs/taggedWorld.hpp"
  "ecs/taggedWorld.cpp"
  "ecs/iassetLoader.hpp")

target_sources(
  RCommon PUBLIC
  "box2d/TaskScheduler.cpp"
  "box2d/TaskScheduler.hpp"
  "box2d/LockLessMultiReadPipe.hpp"
  "box2d/box2dScheduler.hpp")

target_sources(
  RCommon PUBLIC
  "common.hpp"
  "commondef.hpp"
  "math.hpp"
  "imodule.hpp"
  "ihost.hpp"
  "pipeline.cpp"
  "pipeline.hpp"
  "hashes.hpp")

file(GLOB schemasCXX "./schema/*.c++" "./schema/*.h")

target_sources(RCommon PUBLIC ${schemasCXX})

add_dependencies(RCommon capnpc_cpp capnp_tool)
