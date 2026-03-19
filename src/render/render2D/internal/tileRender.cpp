#include "tileRender.hpp"

RAMPAGE_START

TileRender::TileRender(VulkanContext& context)
  : VulkanRender(context) {
  vk::DescriptorSetLayoutBinding vpLayoutBinding{};
  vpLayoutBinding.binding = 0;
  vpLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  vpLayoutBinding.descriptorCount = 1;
  vpLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  vpLayoutBinding.pImmutableSamplers = nullptr;
  vk::DescriptorSetLayoutBinding tileSideLengthBinding{};
  tileSideLengthBinding.binding = 1;
  tileSideLengthBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  tileSideLengthBinding.descriptorCount = 1;
  tileSideLengthBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  tileSideLengthBinding.pImmutableSamplers = nullptr;
  vk::DescriptorSetLayoutBinding samplerBinding {};
  samplerBinding.binding = 2;
  samplerBinding.descriptorType = vk::DescriptorType::eSampler;
  samplerBinding.descriptorCount = 1;
  samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
  samplerBinding.pImmutableSamplers = nullptr;
  std::vector<vk::DescriptorSetLayoutBinding> bindings = { vpLayoutBinding, tileSideLengthBinding, samplerBinding };
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = (u32)bindings.size();
  layoutInfo.pBindings = bindings.data();
  m_descSetLayout = m_device.createDescriptorSetLayout(layoutInfo);
}

void TileRender::reset() {

}

void TileRender::process(const std::vector<DrawTileCmd>& cmds) {

}

void TileRender::draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) {

}

bool TileRender::recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) { 
  auto vertShaderModule = loadShaderModuleFromFile(m_device, TILE_RENDER_VERTEX_SHADER_PATH);
  auto fragShaderModule = loadShaderModuleFromFile(m_device, TILE_RENDER_FRAGMENT_SHADER_PATH);

  // Vertex Description
  vk::VertexInputBindingDescription vertexBindingDesc{};
  vertexBindingDesc.binding = 0;                  // binding index
  vertexBindingDesc.stride = sizeof(TileVertex);      // size of one vertex
  vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex; // per-vertex
  vk::VertexInputBindingDescription instanceBindingDesc{};
  instanceBindingDesc.binding = 1;                  // binding index
  instanceBindingDesc.stride = sizeof(TileInstance);      // size of one instance
  instanceBindingDesc.inputRate = vk::VertexInputRate::eInstance; // per-instance
  std::vector<vk::VertexInputBindingDescription> bindingDescs = { vertexBindingDesc, instanceBindingDesc };
  // Attribute descriptions
  std::vector<vk::VertexInputAttributeDescription> attribDescs = {
      // Vertex
      { 0, 0, vk::Format::eR32G32Sfloat, offsetof(TileVertex, pos) },
      { 1, 0, vk::Format::eR32G32Sfloat, offsetof(TileVertex, texCoords) },
      // Instance
      { 2, 1, vk::Format::eR32G32Sfloat, offsetof(TileInstance, localPos) },
      { 3, 1, vk::Format::eR32G32Sfloat, offsetof(TileInstance, worldPos) },
      { 4, 1, vk::Format::eR16Uint,      offsetof(TileInstance, layer) },
      { 5, 1, vk::Format::eR32Sfloat,    offsetof(TileInstance, rot) },
      { 6, 1, vk::Format::eR32Sfloat,    offsetof(TileInstance, z) },
      { 7, 1, vk::Format::eR8Uint,       offsetof(TileInstance, dim) },
      { 8, 1, vk::Format::eR32Sfloat,    offsetof(TileInstance, scale) },
  };
  // Vertex input (binding & attribute descriptions)
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
  vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescs.size());
  vertexInputInfo.pVertexAttributeDescriptions = attribDescs.data();

  // Color blending
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR |
                                      vk::ColorComponentFlagBits::eG |
                                      vk::ColorComponentFlagBits::eB |
                                      vk::ColorComponentFlagBits::eA)
                                      .setBlendEnable(false);
  vk::PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.setAttachments({colorBlendAttachment});
  
  // Pipeline layout (for descriptor sets / push constants)
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &m_descSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

  glm::vec2 screenSize = {
    static_cast<float>(renderTargets->extent.width),
    static_cast<float>(renderTargets->extent.height)
  };

  GraphicsPipelineBuilder builder(m_device);
  builder.setTopology(vk::PrimitiveTopology::eTriangleList)
    .enableDepthTest()
    .setCullMode(vk::CullModeFlagBits::eBack)
    .setShaders(vertShaderModule, fragShaderModule)
    .setRenderPass(renderTargets->mainRenderPass)
    .setScissor(renderTargets->extent, {0, 0})
    .setViewport(0, 0, screenSize.x, screenSize.y, 0.0f, 1.0f)
    .setColorBlendState(colorBlending)
    .setVertexInputState(vertexInputInfo);
  m_pipeline = builder.build(m_pipelineLayout);
  if(!m_pipeline) {
    return false;
  } 
   
  return true;
}

RAMPAGE_END