#pragma once

#include "../commands.hpp"
#include "render.hpp"

RAMPAGE_START

static constexpr const char* SHAPE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/shapevs.spv";
static constexpr const char* SHAPE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/shapefs.spv";

class ShapeRender : public VulkanRender {
  struct ShapeVertex {
    glm::vec3 pos;
    glm::vec3 color;
  };

public:
  ShapeRender(VulkanContext& context);
  ~ShapeRender();

  void reset();
  void process(const std::vector<DrawRectangleCmd>& cmds);
  void process(const std::vector<DrawCircleCmd>& cmds);
  void process(const std::vector<DrawLineCmd>& cmds);
  void process(const std::vector<DrawHollowCircleCmd>& cmds);
  void draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) override;

  bool createVertexBuffer(size_t maxVerticesCount);
  bool recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) override;

  virtual std::vector<NeededDescriptorType> getNeededDescriptorTypes() const override;
  virtual void createDescriptorSets(vk::DescriptorPool pool) override;

private:
  // Uniforms.
  vk::DescriptorSet m_descSet = {};
  vk::DescriptorSetLayout m_descSetLayout;
  VkBuffer m_uvpBuffer = nullptr;
  VmaAllocation m_uvpAllocation = nullptr; 
  // Pipeline
  vk::PipelineLayout m_pipelineLayout;
  vk::Pipeline m_pipeline;
  // Vulkan Buffers
  VkBuffer m_vertexBuffer = nullptr;
  VmaAllocation m_vertexAllocation = nullptr;
  size_t m_maxVerticesCount = 0;
  // CPU-Side Vertices
  std::vector<ShapeVertex> m_vertices;
  std::vector<ShapeVertex> m_baseCircleMesh;

  void generateCircleMesh(int resolution);
  void addLine(const DrawLineCmd& cmd);
};

RAMPAGE_END