#include "graphicsPipelineBuilder.hpp"

RAMPAGE_START

GraphicsPipelineBuilder::GraphicsPipelineBuilder(vk::Device device)
  : m_device(device) {
  m_inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

  m_rasterizer.polygonMode = vk::PolygonMode::eFill;
  m_rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  m_rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  m_rasterizer.lineWidth = 1.0f;

  m_multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setShaders(vk::ShaderModule vs,vk::ShaderModule fs) {
  m_shaderStages.clear();

  vk::PipelineShaderStageCreateInfo vert{};
  vert.stage = vk::ShaderStageFlagBits::eVertex;
  vert.module = vs;
  vert.pName = "main";

  vk::PipelineShaderStageCreateInfo frag{};
  frag.stage = vk::ShaderStageFlagBits::eFragment;
  frag.module = fs;
  frag.pName = "main";

  m_shaderStages.push_back(vert);
  m_shaderStages.push_back(frag);

  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setRenderPass(vk::RenderPass pass) {
  m_renderPass = pass;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setTopology(vk::PrimitiveTopology topology) {
  m_inputAssembly.topology = topology;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::enableDepthTest() {
  m_depthStencil.depthTestEnable = true;
  m_depthStencil.depthWriteEnable = true;
  m_depthStencil.depthCompareOp = vk::CompareOp::eLess;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setCullMode(vk::CullModeFlags mode) {
  m_rasterizer.cullMode = mode;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
  m_viewport = vk::Viewport{x, y, width, height, minDepth, maxDepth};
  m_viewportState.viewportCount = 1;
  m_viewportState.pViewports = &m_viewport;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  m_scissor = vk::Rect2D{vk::Offset2D{x, y}, vk::Extent2D{width, height}};
  m_viewportState.scissorCount = 1;
  m_viewportState.pScissors = &m_scissor;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setScissor(const vk::Extent2D& extent, const vk::Offset2D& offset) {
  m_scissor = vk::Rect2D{offset, extent};
  m_viewportState.scissorCount = 1;
  m_viewportState.pScissors = &m_scissor;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setColorBlendState(const vk::PipelineColorBlendStateCreateInfo& colorBlendState) {
  m_colorBlend = colorBlendState;
  return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::setVertexInputState(const vk::PipelineVertexInputStateCreateInfo& vertexInputState) {
  m_vertexInput = vertexInputState;
  return *this;
}

vk::Pipeline GraphicsPipelineBuilder::build(vk::PipelineLayout layout) {
  vk::GraphicsPipelineCreateInfo info{};

  info.stageCount = static_cast<uint32_t>(m_shaderStages.size());
  info.pStages = m_shaderStages.data();

  info.pVertexInputState = &m_vertexInput;
  info.pInputAssemblyState = &m_inputAssembly;
  info.pViewportState = &m_viewportState;
  info.pRasterizationState = &m_rasterizer;
  info.pMultisampleState = &m_multisampling;
  info.pDepthStencilState = &m_depthStencil;
  info.pColorBlendState = &m_colorBlend;

  info.layout = layout;
  info.renderPass = m_renderPass;
  info.subpass = 0;

  auto result = m_device.createGraphicsPipeline(nullptr, info);

  if(result.result != vk::Result::eSuccess)
      throw std::runtime_error("Pipeline creation failed");

  return result.value;
}

RAMPAGE_END