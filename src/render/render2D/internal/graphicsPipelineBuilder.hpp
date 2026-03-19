#pragma once

#include "vkcommon.hpp"

RAMPAGE_START

class GraphicsPipelineBuilder {
public:
  GraphicsPipelineBuilder(vk::Device device);

  GraphicsPipelineBuilder& setShaders(vk::ShaderModule vs, vk::ShaderModule fs);
  GraphicsPipelineBuilder& setRenderPass(vk::RenderPass renderPass);
  GraphicsPipelineBuilder& setTopology(vk::PrimitiveTopology topology);
  GraphicsPipelineBuilder& setCullMode(vk::CullModeFlags mode);
  GraphicsPipelineBuilder& enableDepthTest();
  GraphicsPipelineBuilder& setViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
  GraphicsPipelineBuilder& setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);
  GraphicsPipelineBuilder& setScissor(const vk::Extent2D& extent, const vk::Offset2D& offset);
  GraphicsPipelineBuilder& setColorBlendState(const vk::PipelineColorBlendStateCreateInfo& colorBlendState);
  GraphicsPipelineBuilder& setVertexInputState(const vk::PipelineVertexInputStateCreateInfo& vertexInputState);

  vk::Pipeline build(vk::PipelineLayout layout);

private:
  vk::Device m_device;

  std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

  vk::PipelineVertexInputStateCreateInfo m_vertexInput{};
  vk::PipelineInputAssemblyStateCreateInfo m_inputAssembly{};
  vk::PipelineViewportStateCreateInfo m_viewportState{};
  vk::PipelineRasterizationStateCreateInfo m_rasterizer{};
  vk::PipelineMultisampleStateCreateInfo m_multisampling{};
  vk::PipelineDepthStencilStateCreateInfo m_depthStencil{};
  vk::PipelineColorBlendStateCreateInfo m_colorBlend{};
  vk::Viewport m_viewport{};
  vk::Rect2D m_scissor{};
  vk::RenderPass m_renderPass{};
};

class GraphicsBuffer {
  GraphicsBuffer(const GraphicsBuffer& buffer) = delete;

public:
  GraphicsBuffer() = default;

  GraphicsBuffer(VmaAllocator allocator, const VkBufferCreateInfo& bufferCreateInfo, const VmaAllocationCreateInfo& allocationCreateInfo) {
    m_allocator = allocator;
    VkResult result = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_allocation, nullptr);
    if(result != VK_SUCCESS) {
      *this = GraphicsBuffer();
    } 
  }

  ~GraphicsBuffer() {
    destroy();
  }

  GraphicsBuffer(GraphicsBuffer&& buffer) {
    m_allocator = buffer.m_allocator;
    m_buffer = buffer.m_buffer;
    m_allocation = buffer.m_allocation;
    buffer.m_allocator = nullptr;
    buffer.m_buffer = nullptr;
    buffer.m_allocation = nullptr;
  }

  GraphicsBuffer& operator=(GraphicsBuffer&& buffer) {
    m_allocator = buffer.m_allocator;
    m_buffer = buffer.m_buffer;
    m_allocation = buffer.m_allocation;
    buffer.m_allocator = nullptr;
    buffer.m_buffer = nullptr;
    buffer.m_allocation = nullptr;
    return *this;
  }

  void destroy() {
    if(m_buffer) {
      vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
  }

  bool isValid() {
    return m_buffer != nullptr;
  }

  operator VkBuffer() {
    return m_buffer;
  }

  operator VmaAllocation() {
    return m_allocation;
  }
  
private:
  VmaAllocator m_allocator = nullptr;
  VkBuffer m_buffer = nullptr;
  VmaAllocation m_allocation = nullptr;
};

RAMPAGE_END