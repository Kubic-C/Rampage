add_library(RRender STATIC "module.hpp" "module.cpp")
set_property(TARGET RRender PROPERTY CXX_STANDARD 20)

target_sources(RRender PUBLIC
  "render2D/stb_image.h"
  "render2D/stb_image.c"
  "render2D/render2D.hpp"
  "render2D/render2D.cpp"
  "render2D/common.hpp"
  "render2D/commands.hpp"
  "camera.hpp"
  "camera.cpp")

target_sources(RRender PUBLIC
  "render2D/internal/internal.hpp"
  "render2D/internal/internal.cpp"
  "render2D/internal/vkcommon.hpp"
  "render2D/internal/graphicsPipelineBuilder.hpp"
  "render2D/internal/graphicsPipelineBuilder.cpp"
  "render2D/internal/context.hpp"
  "render2D/internal/context.cpp"
  "render2D/internal/render.hpp"
  "render2D/internal/graphicsPipelineBuilder.hpp"
  "render2D/internal/graphicsPipelineBuilder.cpp"
  "render2D/internal/shapeRender.hpp"
  "render2D/internal/shapeRender.cpp"
  "render2D/internal/tileRender.hpp"
  "render2D/internal/tileRender.cpp")

target_link_libraries(RRender PUBLIC RCommon)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Include Vulkan headers
target_include_directories(RRender PUBLIC ${Vulkan_INCLUDE_DIRS})

# Link Vulkan library
target_link_libraries(RRender PUBLIC ${Vulkan_LIBRARIES} VulkanMemoryAllocator)