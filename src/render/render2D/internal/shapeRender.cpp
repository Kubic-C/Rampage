#include "shapeRender.hpp"

RAMPAGE_START

void ShapeRender::generateCircleMesh(int resolution) {
  int count = 0;
  const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
  float x = 1;
  float y = 0;
  for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
    float xNext = cos(angle + anglePerTriangle);
    float yNext = sin(angle + anglePerTriangle);

    m_baseCircleMesh.push_back(ShapeVertex({0, 0, 0}, glm::vec3(0, 0, 0)));
    m_baseCircleMesh.push_back(ShapeVertex({x, y, 0}, glm::vec3(0, 0, 0)));
    m_baseCircleMesh.push_back(ShapeVertex({xNext, yNext, 0}, glm::vec3(0, 0, 0)));

    x = xNext;
    y = yNext;
  }
}

void ShapeRender::addLine(const DrawLineCmd& cmd) {
  float l = glm::length(cmd.to - cmd.from);
  glm::vec2 dir = glm::normalize(cmd.to - cmd.from);
  float angle = atan2(dir.y, dir.x);

  glm::vec3 rect[4] = {
    glm::vec3(0, cmd.thickness, cmd.z), 
    glm::vec3(0, -cmd.thickness, cmd.z), 
    glm::vec3(l, -cmd.thickness, cmd.z), 
    glm::vec3(l, cmd.thickness, cmd.z)};

  for (int i = 0; i < 4; i++) {
    rect[i] = glm::rotate(rect[i], angle, glm::vec3(0.0, 0.0f, 1.0f));
    rect[i] += glm::vec3(cmd.from, 0.0f);
  }

  std::array<ShapeVertex, 6> vertices = {
    ShapeVertex(rect[0], cmd.color), 
    ShapeVertex(rect[1], cmd.color), 
    ShapeVertex(rect[2], cmd.color),
    ShapeVertex(rect[2], cmd.color), 
    ShapeVertex(rect[3], cmd.color), 
    ShapeVertex(rect[0], cmd.color)
  };

  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
}

ShapeRender::ShapeRender(VulkanContext& context)
  : VulkanRender(context) {
  generateCircleMesh(36); 
  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0; // matches layout(binding = 0) in GLSL
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;
  m_descSetLayout = m_device.createDescriptorSetLayout(layoutInfo);
}

ShapeRender::~ShapeRender() {
  if(m_descSetLayout)
    m_device.destroyDescriptorSetLayout(m_descSetLayout);
  if(m_uvpAllocation) {
    vmaDestroyBuffer(m_allocator, m_uvpBuffer, m_uvpAllocation);
  }
  if(m_pipelineLayout) {
    m_device.destroyPipelineLayout(m_pipelineLayout);
  }
  if(m_pipeline) {
    m_device.destroyPipeline(m_pipeline);
  }
  if(m_vertexAllocation) {
    vmaDestroyBuffer(m_allocator, m_vertexBuffer, m_vertexAllocation);
  }
}

void ShapeRender::reset() {
  m_vertices.clear();
}

void ShapeRender::process(const std::vector<DrawRectangleCmd>& cmds) {
  for(const auto& cmd : cmds) {
    Transform transform(cmd.pos, cmd.rot);
  
    const glm::vec3 rect[4] = {
      glm::vec3(-cmd.halfSize.x, -cmd.halfSize.y, cmd.z), 
      glm::vec3( cmd.halfSize.x, -cmd.halfSize.y, cmd.z),
      glm::vec3( cmd.halfSize.x,  cmd.halfSize.y, cmd.z),          
      glm::vec3(-cmd.halfSize.x,  cmd.halfSize.y, cmd.z)
    };

    std::array<ShapeVertex, 6> vertices = {
      ShapeVertex(rect[0], cmd.color), 
      ShapeVertex(rect[1], cmd.color), 
      ShapeVertex(rect[2], cmd.color),
      ShapeVertex(rect[2], cmd.color), 
      ShapeVertex(rect[3], cmd.color), 
      ShapeVertex(rect[0], cmd.color)};

    for (int i = 0; i < 6; i++) {
      vertices[i].pos =
        glm::vec3(transform.getWorldPoint({vertices[i].pos.x, vertices[i].pos.y}), vertices[i].pos.z);
    }

    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
  }
}

void ShapeRender::process(const std::vector<DrawCircleCmd>& cmds) {
  for(const auto& cmd : cmds) {
    for (int i = 0; i < m_baseCircleMesh.size(); i += 3) {
      Transform transform(cmd.pos, 0);

      ShapeVertex copy[3];
      std::memcpy(copy, &m_baseCircleMesh[i], sizeof(ShapeVertex) * 3);

      for (int i = 0; i < 3; i++) {
        copy[i].pos = glm::vec3(transform.getWorldPoint({copy[i].pos.x * cmd.radius, copy[i].pos.y * cmd.radius}), cmd.z);
        copy[i].color = cmd.color;
      }

      m_vertices.insert(m_vertices.end(), copy, copy + 3);
    }
  }
}

void ShapeRender::process(const std::vector<DrawLineCmd>& cmds) {
  for(const auto& cmd : cmds) {
    addLine(cmd);
  }
}

void ShapeRender::process(const std::vector<DrawHollowCircleCmd>& cmds) {
  for(const auto& cmd : cmds) {
    int count = 0;
    const float anglePerTriangle = glm::pi<float>() * 2 / cmd.resolution;
    float x = cmd.radius;
    float y = 0;
    for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
      float xNext = cos(angle + anglePerTriangle) * cmd.radius;
      float yNext = sin(angle + anglePerTriangle) * cmd.radius;

      DrawLineCmd line;
      line.from = Vec2(x, y) + cmd.pos;
      line.to = Vec2(xNext, yNext) + cmd.pos;
      line.thickness = cmd.thickness;
      line.color = cmd.color;
      addLine(line);

      x = xNext;
      y = yNext;
    }
  }
}

void ShapeRender::draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) {
  if(m_vertices.empty()) return;

  void* uvpMapPtr = nullptr;
  vmaMapMemory(m_allocator, m_uvpAllocation, &uvpMapPtr);
  std::memcpy(uvpMapPtr, glm::value_ptr(vp), sizeof(glm::mat4));
  vmaUnmapMemory(m_allocator, m_uvpAllocation);

  if (m_vertices.size() > m_maxVerticesCount) {
    createVertexBuffer(m_vertices.size() * 2);
  }

  void* mapped = nullptr;
  vmaMapMemory(m_allocator, m_vertexAllocation, &mapped);
  memcpy(mapped, m_vertices.data(), m_vertices.size() * sizeof(ShapeVertex));
  vmaUnmapMemory(m_allocator, m_vertexAllocation);

  cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
  cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, {m_descSet}, nullptr);
  vk::DeviceSize offsets[] = {0};
  cmdBuffer.bindVertexBuffers(0, {m_vertexBuffer}, offsets);
  cmdBuffer.draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
}

bool ShapeRender::createVertexBuffer(size_t maxVerticesCount) {
  if(m_vertexBuffer) {
    vmaDestroyBuffer(m_allocator, m_vertexBuffer, m_vertexAllocation);
  }

  VkDeviceSize size = maxVerticesCount * sizeof(ShapeVertex);
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  allocInfo.requiredFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_vertexBuffer, &m_vertexAllocation, nullptr) != VK_SUCCESS) {
      return false;
  }

  m_maxVerticesCount = maxVerticesCount;

  return true;
}

bool ShapeRender::recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) {
  auto vertShaderModule = loadShaderModuleFromFile(m_device, SHAPE_RENDER_VERTEX_SHADER_PATH);
  auto fragShaderModule = loadShaderModuleFromFile(m_device, SHAPE_RENDER_FRAGMENT_SHADER_PATH);

  // Binding description
  vk::VertexInputBindingDescription bindingDesc{};
  bindingDesc.binding = 0;                  // binding index
  bindingDesc.stride = sizeof(ShapeVertex);      // size of one vertex
  bindingDesc.inputRate = vk::VertexInputRate::eVertex; // per-vertex
  std::vector<vk::VertexInputBindingDescription> bindingDescs = { bindingDesc };
  // Attribute descriptions
  std::vector<vk::VertexInputAttributeDescription> attribDescs = {
      { 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(ShapeVertex, pos) },   // location 0 -> pos
      { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(ShapeVertex, color) }, // location 1 -> color
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

std::vector<NeededDescriptorType> ShapeRender::getNeededDescriptorTypes() const {
  return {
    {vk::DescriptorType::eUniformBuffer, 1}
  };
}

void ShapeRender::createDescriptorSets(vk::DescriptorPool pool) {
  vk::BufferCreateInfo uniformBufferInfo;
  uniformBufferInfo.size = sizeof(glm::mat4);
  uniformBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
  uniformBufferInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo uniformAllocInfo{};
  uniformAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  uniformAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  uniformAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  vmaCreateBuffer(m_allocator, uniformBufferInfo, &uniformAllocInfo, &m_uvpBuffer, &m_uvpAllocation, nullptr);

  vk::DescriptorSetAllocateInfo allocInfoDS{};
  allocInfoDS.descriptorPool = pool;
  allocInfoDS.descriptorSetCount = 1;
  allocInfoDS.pSetLayouts = &m_descSetLayout;
  m_descSet = m_device.allocateDescriptorSets(allocInfoDS)[0];

  // This sets the pointer in the descriptor set
  vk::DescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = m_uvpBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(glm::mat4);
  vk::WriteDescriptorSet descriptorWrite{};
  descriptorWrite.dstSet = m_descSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;
  m_device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}

RAMPAGE_END