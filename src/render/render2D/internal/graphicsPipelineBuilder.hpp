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

RAMPAGE_END