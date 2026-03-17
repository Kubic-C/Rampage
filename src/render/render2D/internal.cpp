#include "internal.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

RAMPAGE_START

static constexpr const char* SHAPE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/shapevs.spv";
static constexpr const char* SHAPE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/shapefs.spv";
static constexpr const char* TILE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/tilevs.spv";
static constexpr const char* TILE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/tilefs.spv";
static constexpr const char* LIGHT_RENDER_VERTEX_SHADER_PATH = "./res/shaders/lightvs.spv";
static constexpr const char* LIGHT_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/lightfs.spv";

u32 Swapchain::nextImage(VulkanContext& context) {
  u32 imageIndex;
  context.getDevice().acquireNextImageKHR(swapchain, UINT64_MAX, m_imageAvailableSemaphore, nullptr, &imageIndex);
  return imageIndex;
}

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char>& code) {
  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  return device.createShaderModule(createInfo);
}

// Reads a binary file and returns a vk::UniqueShaderModule
vk::ShaderModule loadShaderModuleFromFile(vk::Device device, const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open shader file: " + filename);
  }
  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return createShaderModule(device, buffer);
}

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
  : m_context(context) {
  generateCircleMesh(36); 
}

ShapeRender::~ShapeRender() {
  auto& device = m_context.getDevice();
  auto allocator = m_context.getAllocator();

  if(m_descPool)
    device.destroyDescriptorPool(m_descPool);
  if(m_descSetLayout)
    device.destroyDescriptorSetLayout(m_descSetLayout);
  if(m_uvpAllocation) {
    vmaDestroyBuffer(allocator, m_uvpBuffer, m_uvpAllocation);
  }
  if(m_pipelineLayout) {
    device.destroyPipelineLayout(m_pipelineLayout);
  }
  if(m_pipeline) {
    device.destroyPipeline(m_pipeline);
  }
  if(m_vertexAllocation) {
    vmaDestroyBuffer(allocator, m_vertexBuffer, m_vertexAllocation);
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

void ShapeRender::draw(DrawInfo& drawInfo) {
  auto allocator = m_context.getAllocator();
  auto& device = m_context.getDevice();
  if(m_vertices.empty()) return;

  void* uvpMapPtr = nullptr;
  vmaMapMemory(allocator, m_uvpAllocation, &uvpMapPtr);
  std::memcpy(uvpMapPtr, glm::value_ptr(drawInfo.vp), sizeof(glm::mat4));
  vmaUnmapMemory(allocator, m_uvpAllocation);

  if (m_vertices.size() > m_maxVerticesCount) {
    createVertexBuffer(m_vertices.size() * 2);
  }

  void* mapped = nullptr;
  vmaMapMemory(allocator, m_vertexAllocation, &mapped);
  memcpy(mapped, m_vertices.data(), m_vertices.size() * sizeof(ShapeVertex));
  vmaUnmapMemory(allocator, m_vertexAllocation);

  drawInfo.cmdBuffer.reset();
  vk::CommandBufferBeginInfo beginInfo{};
  drawInfo.cmdBuffer.begin(beginInfo);
  vk::ClearValue clearValues[2];
  clearValues[0].color = vk::ClearColorValue(std::array<float,4>{0.f,0.f,0.f,1.f});
  clearValues[1].depthStencil.depth = 1.0f;
  clearValues[1].depthStencil.stencil = 0;
  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = drawInfo.mainRenderPass;
  renderPassInfo.framebuffer = drawInfo.framebuffer;
  renderPassInfo.renderArea.extent = drawInfo.extent;
  renderPassInfo.clearValueCount = 2;
  renderPassInfo.pClearValues = clearValues;
  drawInfo.cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  drawInfo.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
  drawInfo.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_descSets[0], nullptr);
  vk::DeviceSize offsets[] = {0};
  drawInfo.cmdBuffer.bindVertexBuffers(0, {m_vertexBuffer}, offsets);
  drawInfo.cmdBuffer.draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
  drawInfo.cmdBuffer.endRenderPass();
  drawInfo.cmdBuffer.end();
}

bool ShapeRender::createVertexBuffer(size_t maxVerticesCount) {
  auto allocator = m_context.getAllocator();

  if(m_vertexBuffer) {
    vmaDestroyBuffer(allocator, m_vertexBuffer, m_vertexAllocation);
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
  if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &m_vertexBuffer, &m_vertexAllocation, nullptr) != VK_SUCCESS) {
      return false;
  }

  m_maxVerticesCount = maxVerticesCount;

  return true;
}

bool ShapeRender::createPipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) {
  auto& device = m_context.getDevice();

  vk::BufferCreateInfo uniformBufferInfo;
  uniformBufferInfo.size = sizeof(glm::mat4);
  uniformBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
  uniformBufferInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo uniformAllocInfo{};
  uniformAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  uniformAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  uniformAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  VkResult uniformBufferSuccess = vmaCreateBuffer(m_context.getAllocator(), uniformBufferInfo, &uniformAllocInfo, &m_uvpBuffer, &m_uvpAllocation, nullptr);
  if(uniformBufferSuccess != VK_SUCCESS) {
    return false;
  }

  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0; // matches layout(binding = 0) in GLSL
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  uboLayoutBinding.pImmutableSamplers = nullptr;
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;
  m_descSetLayout = device.createDescriptorSetLayout(layoutInfo);

  vk::DescriptorPoolSize poolSize{};
  poolSize.type = vk::DescriptorType::eUniformBuffer;
  poolSize.descriptorCount = 1;
  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;
  m_descPool = device.createDescriptorPool(poolInfo);
  vk::DescriptorSetAllocateInfo allocInfoDS{};
  allocInfoDS.descriptorPool = m_descPool;
  allocInfoDS.descriptorSetCount = 1;
  allocInfoDS.pSetLayouts = &m_descSetLayout;
  m_descSets = device.allocateDescriptorSets(allocInfoDS);

  vk::DescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = m_uvpBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(glm::mat4);
  vk::WriteDescriptorSet descriptorWrite{};
  descriptorWrite.dstSet = m_descSets[0];
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;
  device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);

  auto vertShaderModule = loadShaderModuleFromFile(device, SHAPE_RENDER_VERTEX_SHADER_PATH);
  auto fragShaderModule = loadShaderModuleFromFile(device, SHAPE_RENDER_FRAGMENT_SHADER_PATH);
  vk::PipelineShaderStageCreateInfo vertStage{};
  vertStage.stage = vk::ShaderStageFlagBits::eVertex;
  vertStage.module = vertShaderModule;
  vertStage.pName = "main";
  vk::PipelineShaderStageCreateInfo fragStage{};
  fragStage.stage = vk::ShaderStageFlagBits::eFragment;
  fragStage.module = fragShaderModule;
  fragStage.pName = "main";
  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStage, fragStage };

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

  // Input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  // Viewport & scissor
  vk::PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;
  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(renderTargets->extent.width);
  viewport.height = static_cast<float>(renderTargets->extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vk::Rect2D scissor{};
  scissor.extent = renderTargets->extent;
  viewportState.pViewports = &viewport;
  viewportState.pScissors = &scissor;
  // Rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.polygonMode = vk::PolygonMode::eFill;
  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  // Multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
  // Depth/stencil
  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = vk::CompareOp::eLess;
  // Color blending
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                      vk::ColorComponentFlagBits::eG |
                                      vk::ColorComponentFlagBits::eB |
                                      vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;
  vk::PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  // Pipeline layout (for descriptor sets / push constants)
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &m_descSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  m_pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
  // Graphics pipeline creation
  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineInfo.pStages = shaderStages.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = m_pipelineLayout;
  pipelineInfo.renderPass = renderTargets->mainRenderPass;
  pipelineInfo.subpass = 0;
  auto pipeline = device.createGraphicsPipeline(nullptr, pipelineInfo);
  if(pipeline.result != vk::Result::eSuccess) {
    return false;
  } else {
    m_pipeline = std::move(pipeline.value);
  }
   
  return true;
}

TileRender::TileRender() {

}

void TileRender::reset() {

}

void TileRender::process(const std::vector<DrawTileCmd>& cmds) {

}

void TileRender::draw() {

}

LightRender::LightRender() {

}

void LightRender::reset() {

}

void LightRender::process(const std::vector<DrawLightCmd>& cmds) {

}

void LightRender::draw() {

}

VulkanContext::VulkanContext(SDL_Window* window, bool enableValidationLayers)
  : m_window(window) {
  if(enableValidationLayers && !isValidationLayersSupported()) {
    markCriticalError();
    return;
  }

  u32 sdlNeededExtensionsCount = 0;
  auto sdlNeededExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlNeededExtensionsCount);
  neededExtensions.insert(neededExtensions.end(), sdlNeededExtensions, sdlNeededExtensions + sdlNeededExtensionsCount);

  if(!isExtensionsSupported()) {
    markCriticalError();
    return;
  }

  vk::ApplicationInfo appInfo;
  appInfo.pApplicationName = SDL_GetWindowTitle(window);
  appInfo.applicationVersion = vk::makeVersion(1, 0, 0);
  appInfo.pEngineName = "Rampage Engine";
  appInfo.engineVersion = vk::makeVersion(1, 0, 0);
  appInfo.apiVersion = vk::ApiVersion14;
  vk::InstanceCreateInfo instanceCreateInfo;
  instanceCreateInfo.pApplicationInfo = &appInfo;
  if(enableValidationLayers) {
    instanceCreateInfo.enabledLayerCount = (u32)neededValidationLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = neededValidationLayers.data();
  }
  instanceCreateInfo.enabledExtensionCount = (u32)neededExtensions.size();
  instanceCreateInfo.ppEnabledExtensionNames = neededExtensions.size() != 0 ? neededExtensions.data() : nullptr;
  m_instance = vk::createInstance(instanceCreateInfo);
  if(!m_instance) {
    markCriticalError();
    return;
  }
  if(!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface)) {
    markCriticalError();
    return;
  }

  m_selDevice = findSuitablePhysicalDevice();
  if(m_selDevice.deviceIdx == std::numeric_limits<u32>::max()) {
    markCriticalError();
    return;
  }

  auto selPhysicalDevice = m_instance.enumeratePhysicalDevices()[m_selDevice.deviceIdx];
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = { m_selDevice.graphicsQueueIdx, m_selDevice.presentQueueIdx };
  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueInfo;
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1; // usually 1 queue per family
    queueInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueInfo);
  }
  vk::PhysicalDeviceFeatures deviceFeatures;
  vk::DeviceCreateInfo deviceCreateInfo;
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.enabledExtensionCount = (u32)neededDeviceExtensions.size();
  deviceCreateInfo.ppEnabledExtensionNames = neededDeviceExtensions.data();
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  m_device = selPhysicalDevice.createDevice(deviceCreateInfo);
  if(!m_device) {
    markCriticalError();
    return;
  }
  m_graphicsQueue = m_device.getQueue(m_selDevice.graphicsQueueIdx, 0);
  m_presentQueue  = m_device.getQueue(m_selDevice.presentQueueIdx, 0);
  m_physicalDevice = selPhysicalDevice;

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
  
  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
  allocatorCreateInfo.physicalDevice = m_physicalDevice;
  allocatorCreateInfo.device = m_device;
  allocatorCreateInfo.instance = m_instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
  if(vmaCreateAllocator(&allocatorCreateInfo, &m_allocator) != VK_SUCCESS) {
    markCriticalError();
    return;
  }

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.queueFamilyIndex = m_selDevice.graphicsQueueIdx;
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  m_commandPool = m_device.createCommandPool(poolInfo);
}

VulkanContext::~VulkanContext() {
  if(m_allocator) {
    vmaDestroyAllocator(m_allocator);
  }


  m_device.destroyCommandPool(m_commandPool);
  m_device.destroy();
  m_instance.destroy();
}

std::shared_ptr<Swapchain> VulkanContext::createSwapchain(u32 width, u32 height, vk::PresentModeKHR presentMode) {
  std::shared_ptr<Swapchain> swapchain = std::make_shared<Swapchain>();

  swapchain->device = m_device;
  auto capabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);

  if(capabilities.currentExtent.width != UINT32_MAX) {
    swapchain->extent = capabilities.currentExtent;
  } else {
    swapchain->extent.width = std::clamp(
      width,
      capabilities.minImageExtent.width,
      capabilities.maxImageExtent.width);

    swapchain->extent.height = std::clamp(
      height,
      capabilities.minImageExtent.height,
      capabilities.maxImageExtent.height);
  }

  swapchain->imageFormat = neededSurfaceFormat;
  
  u32 imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo;
  createInfo.surface = m_surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = swapchain->imageFormat;
  createInfo.imageExtent = swapchain->extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  uint32_t queueFamilyIndices[] = {
      m_selDevice.graphicsQueueIdx,
      m_selDevice.presentQueueIdx
  };
  if (m_selDevice.graphicsQueueIdx != m_selDevice.presentQueueIdx) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
      createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = presentMode;
  createInfo.clipped = true;
  swapchain->swapchain = m_device.createSwapchainKHR(createInfo);
  if(!swapchain->swapchain)
    return nullptr;

  // COLOR ATTACHMENTS
  swapchain->images = m_device.getSwapchainImagesKHR(swapchain->swapchain);

  vk::SemaphoreCreateInfo semaphoreInfo{};

  swapchain->m_imageAvailableSemaphore = m_device.createSemaphore(semaphoreInfo);
  swapchain->m_renderFinishedSemaphore = m_device.createSemaphore(semaphoreInfo);

  return swapchain;
}

std::shared_ptr<SwapchainRenderTargets> VulkanContext::createSwapchainRenderTargets(const std::shared_ptr<Swapchain>& swapchain, const std::shared_ptr<SwapchainRenderTargets>& previous) {
  std::shared_ptr<SwapchainRenderTargets> renderTargets;
  if(!previous || swapchain->extent != previous->extent) {
    renderTargets = std::make_unique<SwapchainRenderTargets>();
    renderTargets->extent = swapchain->extent;
    renderTargets->allocator = m_allocator;
    renderTargets->device = m_device;
    renderTargets->pool = m_commandPool;

    // DEPTH ATTACHMENTS
    vk::ImageCreateInfo depthImageInfo{};
    depthImageInfo.imageType = vk::ImageType::e2D;
    depthImageInfo.format = vk::Format::eD32SfloatS8Uint;
    depthImageInfo.extent.width = swapchain->extent.width;
    depthImageInfo.extent.height = swapchain->extent.height;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = vk::SampleCountFlagBits::e1;
    depthImageInfo.tiling = vk::ImageTiling::eOptimal;
    depthImageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depthImageInfo.sharingMode = vk::SharingMode::eExclusive;
    depthImageInfo.initialLayout = vk::ImageLayout::eUndefined;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    for (size_t i = 0; i < swapchain->images.size(); i++) {
      VkImage depthImage;
      VmaAllocation depthImageAlloc;
      VkResult result = vmaCreateImage(m_allocator, depthImageInfo, &allocInfo, &depthImage, &depthImageAlloc, nullptr);

      renderTargets->depthImageMemories.push_back(depthImageAlloc);
      renderTargets->depthImages.push_back(depthImage);

      // Create image view for depth
      vk::ImageViewCreateInfo depthViewInfo{};
      depthViewInfo.image = renderTargets->depthImages.back();
      depthViewInfo.viewType = vk::ImageViewType::e2D;
      depthViewInfo.format = vk::Format::eD32SfloatS8Uint;
      depthViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
      depthViewInfo.subresourceRange.baseMipLevel = 0;
      depthViewInfo.subresourceRange.levelCount = 1;
      depthViewInfo.subresourceRange.baseArrayLayer = 0;
      depthViewInfo.subresourceRange.layerCount = 1;
      renderTargets->depthImageViews.push_back(m_device.createImageView(depthViewInfo));

      if(!renderTargets->depthImageMemories[i] || 
        !renderTargets->depthImages[i] || 
        !renderTargets->depthImageViews[i]) {
          return nullptr;
      }
    }
  } else {
    renderTargets = previous;
    renderTargets->mainRenderPass = nullptr;
    renderTargets->imageViews.resize(0);
    renderTargets->framebuffers.resize(0);
    renderTargets->framebuffers.resize(0);
    renderTargets->commandBuffers.resize(0);
  }
  for (auto image : swapchain->images) {
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = swapchain->imageFormat;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    renderTargets->imageViews.push_back(m_device.createImageView(viewInfo));
  }
  
  vk::AttachmentDescription colorAttachment{};
  colorAttachment.format = swapchain->imageFormat;
  colorAttachment.samples = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;   // clear at start
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore; // store result to present
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  vk::AttachmentDescription depthAttachment{};
  depthAttachment.format = vk::Format::eD32SfloatS8Uint;
  depthAttachment.samples = vk::SampleCountFlagBits::e1;
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  // Reference for subpass
  vk::AttachmentReference colorRef{};
  colorRef.attachment = 0; // index in attachment array
  colorRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference depthRef{};
  depthRef.attachment = 1;
  depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  // Subpass description
  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;
  // Render pass creation
  std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
  vk::RenderPassCreateInfo renderPassInfo{};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderTargets->mainRenderPass = m_device.createRenderPass(renderPassInfo);
  if(!renderTargets->mainRenderPass)
    return nullptr;

  renderTargets->frameFences.resize(swapchain->images.size());
  for(size_t i = 0; i < swapchain->images.size(); i++) {
    std::array<vk::ImageView, 2> attachments = {
        renderTargets->imageViews[i],  // color
        renderTargets->depthImageViews[i] // depth
    };

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderTargets->mainRenderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = swapchain->extent.width;
    fbInfo.height = swapchain->extent.height;
    fbInfo.layers = 1;
    renderTargets->framebuffers.push_back(m_device.createFramebuffer(fbInfo));

    vk::FenceCreateInfo fenceInfo;
    renderTargets->frameFences[i] = m_device.createFence(fenceInfo);
  }

  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = swapchain->images.size();
  renderTargets->commandBuffers = m_device.allocateCommandBuffers(allocInfo);

  return renderTargets;
}

bool VulkanContext::isValidationLayersSupported() {
  auto validationLayers = vk::enumerateInstanceLayerProperties();
  for(const auto& neededLayer : neededValidationLayers) {
    bool found = false;
    for(const auto& layer : validationLayers) {
      if(std::string(layer.layerName.data()) == std::string_view(neededLayer)) {
        found = true;
        break;
      }
    }

    if(!found)
      return false;
  }

  return true;
}

bool VulkanContext::isExtensionsSupported() {
  auto extensions = vk::enumerateInstanceExtensionProperties();
  for(const auto& neededExtension : neededExtensions) {
    bool found = false;
    for(const auto& extension : extensions) {
      if(std::string(extension.extensionName.data()) == std::string_view(neededExtension)) {
        found = true;
        break;
      }
    }

    if(!found)
      return false;
  }

  return true;
}

bool VulkanContext::isSurfacePresentModesSupported(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
  for(const auto& neededPresentMode : neededPresentModes) {
    bool found = false;
    for(const auto& presentMode : availablePresentModes) {
      if(presentMode == neededPresentMode) {
        found = true;
        break;
      }
    }

    if(!found)
      return false;
  }

  return true;
}

VulkanContext::SelectedPhysicalDeviceInfo VulkanContext::findSuitablePhysicalDevice() {
  SelectedPhysicalDeviceInfo shouldUse;

  auto physicalDevices = m_instance.enumeratePhysicalDevices();    
  // Track best discrete and integrated device
  u32 bestDiscreteIdx = std::numeric_limits<u32>::max();
  u32 bestIntegratedIdx = std::numeric_limits<u32>::max();
  u64 bestDiscreteScore = 0;
  u64 bestIntegratedScore = 0;
  for(size_t i = 0; i < physicalDevices.size(); i++) {
    auto& physicalDevice = physicalDevices[i];
    // REQUIRED) Check required device extensions
    size_t foundDeviceExtensions = 0;
    auto deviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for(const auto& neededDeviceExtension : neededDeviceExtensions) {
      for(const auto& deviceExtension : deviceExtensions) {
        if(std::string(deviceExtension.extensionName.data()) == std::string_view(neededDeviceExtension)) {
          foundDeviceExtensions++;
          break;
        }
      }
    }
    if(foundDeviceExtensions != neededDeviceExtensions.size()) {
      continue; // skip devices missing required extensions
    }
    // REQUIRED) Ensure available queue families support needs.
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    u32 graphicsQueueIdx = std::numeric_limits<u32>::max();
    u32 presentQueueIdx = std::numeric_limits<u32>::max();
    for (uint32_t j = 0; j < queueFamilies.size(); j++) {
      auto& q = queueFamilies[j];
      if (q.queueFlags & vk::QueueFlagBits::eGraphics && graphicsQueueIdx == std::numeric_limits<u32>::max())
        graphicsQueueIdx = j;

      // For window presentation
      VkBool32 presentSupport = physicalDevice.getSurfaceSupportKHR(j, m_surface);
      if (presentSupport && presentQueueIdx == std::numeric_limits<u32>::max())
        presentQueueIdx = j;
    }
    if(graphicsQueueIdx == std::numeric_limits<u32>::max() || presentQueueIdx == std::numeric_limits<u32>::max()) {
      continue;
    }

    // REQUIRED) Ensure it has proper surface capabilties.
    bool containsAcceptableFormat = false;
    auto formats = physicalDevice.getSurfaceFormatsKHR(m_surface);
    for(auto& format : formats) {
      if(format.colorSpace == neededColorSpace && format.format == neededSurfaceFormat) {
        containsAcceptableFormat = true;
        break;
      }
    }
    if(!containsAcceptableFormat)
      continue;
    
    // TODO) check surface capabilities.

    // REQUIRED) Should support all present modes;
    if(!isSurfacePresentModesSupported(physicalDevice.getSurfacePresentModesKHR(m_surface)))
      continue;

    vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
    // PREFERENCE) Score device: use maxComputeSharedMemorySize, maxImageDimension2D, deviceID, etc.
    uint64_t score = 0;
    score += props.limits.maxImageDimension2D * 1000;
    score += props.limits.maxComputeSharedMemorySize * 10;
    score += props.limits.maxComputeWorkGroupInvocations * 100;
    if(props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
      if(score > bestDiscreteScore) {
        bestDiscreteScore = score;
        bestDiscreteIdx = static_cast<int>(i);
        shouldUse.graphicsQueueIdx = graphicsQueueIdx;
        shouldUse.presentQueueIdx = presentQueueIdx;
      }
    } else if(bestDiscreteIdx == std::numeric_limits<u32>::max() && props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
      if(score > bestIntegratedScore) {
        bestIntegratedScore = score;
        bestIntegratedIdx = static_cast<int>(i);
        shouldUse.graphicsQueueIdx = graphicsQueueIdx;
        shouldUse.presentQueueIdx = presentQueueIdx;
      }
    }
  }
  // Prefer best discrete, else best integrated
  if(bestDiscreteIdx != std::numeric_limits<u32>::max()) {
    shouldUse.deviceIdx = static_cast<size_t>(bestDiscreteIdx);
  } else if(bestIntegratedIdx != std::numeric_limits<u32>::max()) {
    shouldUse.deviceIdx = static_cast<size_t>(bestIntegratedIdx);
  } 

  if(shouldUse.deviceIdx != std::numeric_limits<u32>::max()) {
    shouldUse.surfaceFormat.colorSpace = neededColorSpace;
    shouldUse.surfaceFormat.format = neededSurfaceFormat;
    shouldUse.memoryProps = physicalDevices[shouldUse.deviceIdx].getMemoryProperties();
  }

  return shouldUse;
}

RAMPAGE_END