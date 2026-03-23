#include "tileRender.hpp"

RAMPAGE_START

std::array<u32, 6> indicies = {0, 1, 2, 2, 3, 0};

void* initDefaultTexture() {
  // Create a 32x32 checkerboard of black and purple
  const int size = 32;
  const int boxSize = 16;  // 16x16 pixel boxes
  const int channels = 4; // RGBA
  u8* data = new u8[size * size * channels];
  
  // Fill with checkerboard pattern
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      int boxX = x / boxSize;
      int boxY = y / boxSize;
      bool isRed = (boxX + boxY) % 2 == 0;
      
      int idx = (y * size + x) * channels;
      if (isRed) {
        data[idx + 0] = 255;     // R - Red
        data[idx + 1] = 0;     // G
        data[idx + 2] = 0;     // B
        data[idx + 3] = 255;   // A
      } else {
        data[idx + 0] = 128;   // R - Purple
        data[idx + 1] = 0;     // G
        data[idx + 2] = 128;   // B
        data[idx + 3] = 255;   // A
      }
    }
  }

  return data;
}

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
  samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  samplerBinding.descriptorCount = 1;
  samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
  samplerBinding.pImmutableSamplers = nullptr;
  std::vector<vk::DescriptorSetLayoutBinding> bindings = { vpLayoutBinding, tileSideLengthBinding, samplerBinding };
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = (u32)bindings.size();
  layoutInfo.pBindings = bindings.data();
  m_descSetLayout = m_device.createDescriptorSetLayout(layoutInfo);

  // uTileSideLength uVP
  vk::BufferCreateInfo uniformBufferInfo;
  uniformBufferInfo.size = sizeof(glm::mat4);
  uniformBufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
  uniformBufferInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo uniformAllocInfo{};
  uniformAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  uniformAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  uniformAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  m_vpBuffer = GraphicsBuffer(m_allocator, uniformBufferInfo, uniformAllocInfo);
  uniformBufferInfo.size = sizeof(float);
  m_tileSideLengthBuffer = GraphicsBuffer(m_allocator, uniformBufferInfo, uniformAllocInfo);
  // uSampler
  vk::ImageCreateInfo tileTextureCreateInfo;
  tileTextureCreateInfo.imageType = vk::ImageType::e2D;
  tileTextureCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
  tileTextureCreateInfo.arrayLayers = maxTileTextures;
  tileTextureCreateInfo.extent = vk::Extent3D(tileTexturePixelSize.x, tileTexturePixelSize.y, 1);
  tileTextureCreateInfo.mipLevels = 1;
  tileTextureCreateInfo.samples = vk::SampleCountFlagBits::e1;
  tileTextureCreateInfo.tiling = vk::ImageTiling::eOptimal;
  tileTextureCreateInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
  tileTextureCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
  VmaAllocationCreateInfo tileTextureAllocInfo{};
  tileTextureAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  tileTextureAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  m_tileTextures = GraphicsImage(m_allocator, tileTextureCreateInfo, tileTextureAllocInfo);
  vk::ImageViewCreateInfo tileTextureViewCreateInfo;
  tileTextureViewCreateInfo.image = m_tileTextures;
  tileTextureViewCreateInfo.viewType = vk::ImageViewType::e2DArray;
  tileTextureViewCreateInfo.format = tileTextureCreateInfo.format;
  tileTextureViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  tileTextureViewCreateInfo.subresourceRange.baseMipLevel = 0;
  tileTextureViewCreateInfo.subresourceRange.levelCount = tileTextureCreateInfo.mipLevels;
  tileTextureViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  tileTextureViewCreateInfo.subresourceRange.layerCount = tileTextureCreateInfo.arrayLayers;
  m_tileTexturesView = m_device.createImageView(tileTextureViewCreateInfo);
  vk::SamplerCreateInfo tileTextureSamplerCreateInfo;
  tileTextureSamplerCreateInfo.magFilter     = vk::Filter::eNearest; 
  tileTextureSamplerCreateInfo.minFilter     = vk::Filter::eNearest;
  tileTextureSamplerCreateInfo.mipmapMode    = vk::SamplerMipmapMode::eLinear;
  tileTextureSamplerCreateInfo.addressModeU  = vk::SamplerAddressMode::eRepeat;
  tileTextureSamplerCreateInfo.addressModeV  = vk::SamplerAddressMode::eRepeat;
  tileTextureSamplerCreateInfo.addressModeW  = vk::SamplerAddressMode::eRepeat;
  tileTextureSamplerCreateInfo.mipLodBias    = 0.0f;
  tileTextureSamplerCreateInfo.anisotropyEnable = false;
  tileTextureSamplerCreateInfo.compareEnable = false;
  tileTextureSamplerCreateInfo.minLod        = 0.0f;
  tileTextureSamplerCreateInfo.maxLod        = vk::LodClampNone;
  tileTextureSamplerCreateInfo.borderColor   = vk::BorderColor::eIntTransparentBlack;
  tileTextureSamplerCreateInfo.unnormalizedCoordinates = false;
  m_tileTextureSampler = m_device.createSampler(tileTextureSamplerCreateInfo);

  // Staging image
  vk::ImageCreateInfo stagingImageCreateInfo;
  stagingImageCreateInfo.imageType = vk::ImageType::e2D;
  stagingImageCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
  stagingImageCreateInfo.arrayLayers = 1;
  stagingImageCreateInfo.extent = vk::Extent3D(tileTexturePixelSize.x, tileTexturePixelSize.y, 1);
  stagingImageCreateInfo.mipLevels = 1;
  stagingImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
  stagingImageCreateInfo.tiling = vk::ImageTiling::eLinear;
  stagingImageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferSrc;
  stagingImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
  VmaAllocationCreateInfo stagingImageAllocInfo{};
  stagingImageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  stagingImageAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  stagingImageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  m_stagingImage = GraphicsImage(m_allocator, stagingImageCreateInfo, stagingImageAllocInfo);

  // Vertex
  vk::BufferCreateInfo tileVertexBufferCreateInfo;
  tileVertexBufferCreateInfo.size = sizeof(TileVertex) * 4;
  tileVertexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
  tileVertexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo tileVertexAllocationInfo{};
  tileVertexAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  tileVertexAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  tileVertexAllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  m_vertexBuffer = GraphicsBuffer(m_allocator, tileVertexBufferCreateInfo, tileVertexAllocationInfo);
  // Index
  vk::BufferCreateInfo tileIndexBufferCreateInfo;
  tileIndexBufferCreateInfo.size = sizeof(u16) * 6;
  tileIndexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
  tileIndexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo tileIndexAllocationInfo{};
  tileIndexAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
  tileIndexAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  tileIndexAllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  m_indexBuffer = GraphicsBuffer(m_allocator, tileIndexBufferCreateInfo, tileIndexAllocationInfo);
  // Instance
  createInstanceBuffer(1000);

  std::array<glm::vec2, 4> tileRect = {
    glm::vec2(-0.5f, -0.5f), 
    glm::vec2(0.5f, -0.5f), 
    glm::vec2(0.5f, 0.5f),
    glm::vec2(-0.5f, 0.5f)
  };

  std::array<glm::vec2, 4> texCoords = {
    glm::vec2(0.0f, 0.0f), 
    glm::vec2(1.0f, 0.0f), 
    glm::vec2(1.0f, 1.0f),
    glm::vec2(0.0f, 1.0f)
  };
  std::array<TileVertex, 4> baseVertexMesh = {
    TileVertex(tileRect[0], texCoords[0]), 
    TileVertex(tileRect[1], texCoords[1]),
    TileVertex(tileRect[2], texCoords[2]), 
    TileVertex(tileRect[3], texCoords[3])
  };
  std::array<u16, 6> indicies = {0, 1, 2, 2, 3, 0};
  m_vertexBuffer.mapAndUpdate(sizeof(baseVertexMesh), baseVertexMesh.data());
  m_indexBuffer.mapAndUpdate(sizeof(indicies), indicies.data());

  std::filesystem::recursive_directory_iterator dirIt("./");
  for (const auto& entry : dirIt) {
    if (!entry.is_regular_file())
      continue;
    
    std::string path = entry.path().string();
    if (isValidImageFile(path)) {
      if(loadSprite(path) == TileTextureId::null()) {
        m_host.log("<bgRed>Failed to load sprite: %s<reset>\n", path.c_str());
      } else {
        m_host.log("Loaded sprite: %s with name <fgYellow>%s<reset>\n", path.c_str(), getFilename(path).c_str());
      }
    }
  }

  { // transition images into correct layout
    size_t imageSize = 32 * 32 * 4;
    void* srcData = initDefaultTexture();
    void* dstData;
    vmaMapMemory(m_allocator, m_stagingImage, &dstData);
    std::memcpy(dstData, srcData, imageSize);
    vmaUnmapMemory(m_allocator, m_stagingImage);
    vmaFlushAllocation(m_allocator, m_stagingImage, 0, imageSize);
    delete[] srcData;

    textureIds["unknown"] = TileTextureId::null();

    vk::FenceCreateInfo fenceCreateInfo;
    vk::Fence fence = m_device.createFence(fenceCreateInfo);

    vk::CommandBufferAllocateInfo cmdAllocInfo;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = m_utilityCmdPool;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmdBuffer = m_device.allocateCommandBuffers(cmdAllocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo;
    cmdBuffer.begin(beginInfo);

    // Transition staging image to TRANSFER_SRC
    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_stagingImage;
    barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    cmdBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {}, barrier);

    // Transition tile array to TRANSFER_DST (you usually want to do *only* the layer you're copying)
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.image = m_tileTextures;
    barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, maxTileTextures);
    cmdBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
      {}, {}, {}, barrier);

    std::vector<vk::ImageCopy> imageCopies(1);
    imageCopies[0].srcOffset = vk::Offset3D(0, 0, 0);
    imageCopies[0].dstOffset = vk::Offset3D(0, 0, 0);
    imageCopies[0].extent = vk::Extent3D(tileTexturePixelSize.x, tileTexturePixelSize.y, 1);
    imageCopies[0].srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopies[0].srcSubresource.baseArrayLayer = 0;
    imageCopies[0].srcSubresource.layerCount = 1;
    imageCopies[0].srcSubresource.mipLevel = 0;
    imageCopies[0].dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopies[0].dstSubresource.layerCount = 1;
    imageCopies[0].dstSubresource.mipLevel = 0;
    for(size_t i = 0; i < maxTileTextures; i++) {
      imageCopies[0].dstSubresource.baseArrayLayer = i;
      cmdBuffer.copyImage((vk::Image)m_stagingImage, vk::ImageLayout::eTransferSrcOptimal, 
                          (vk::Image)m_tileTextures, vk::ImageLayout::eTransferDstOptimal, imageCopies);
    }

    // Transition tile array to TRANSFER_DST (you usually want to do *only* the layer you're copying)
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.image = m_tileTextures;
    barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, maxTileTextures);
    cmdBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eFragmentShader,
      {}, {}, {}, barrier);

    cmdBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    m_utilityQueue.submit(submitInfo, fence);

    std::vector<vk::Fence> waitFences = { fence };
    m_device.waitForFences(waitFences, true, UINT64_MAX);
    m_device.resetFences(waitFences);
    m_device.destroyFence(fence);
  }
}

void TileRender::reset() {
  m_tileInstances.resize(0);
}

void TileRender::process(const std::vector<DrawTileCmd>& cmds) {
  for(auto cmd : cmds) {
    TileInstance& instance = m_tileInstances.emplace_back();
    instance.localPos = cmd.localOffset;
    instance.worldPos = cmd.pos;
    instance.layer = cmd.texture.value();
    instance.scale = cmd.scale;
    instance.z = cmd.z;
    instance.rot = cmd.rot;
    instance.setDim(cmd.size.x, cmd.size.y);
  }
}

void TileRender::draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) {
  if(m_tileInstances.empty()) 
    return;

  if (m_tileInstances.size() > m_maxInstances) {
    createInstanceBuffer(m_tileInstances.size() * 2);
  }

  m_vpBuffer.mapAndUpdate(sizeof(vp), glm::value_ptr(vp));
  float tileSl = 1.0f;
  m_tileSideLengthBuffer.mapAndUpdate(sizeof(float), &tileSl);
  m_instanceBuffer.mapAndUpdate(sizeof(TileInstance) * m_tileInstances.size(), m_tileInstances.data());

  cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
  cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, {m_descSet}, nullptr);
  cmdBuffer.bindVertexBuffers(0, {(vk::Buffer)m_vertexBuffer, (vk::Buffer)m_instanceBuffer}, {0, 0});
  cmdBuffer.bindIndexBuffer((vk::Buffer)m_indexBuffer, {0}, vk::IndexType::eUint16);
  cmdBuffer.drawIndexed(6, m_tileInstances.size(), 0, 0, 0);
}

bool TileRender::createInstanceBuffer(size_t maxInstances) {
  if(m_instanceBuffer.isValid()) {
    m_instanceBuffer.destroy();
  }

  VkDeviceSize size = maxInstances * sizeof(TileInstance);
  vk::BufferCreateInfo bufferInfo;
  bufferInfo.size = size;
  bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;
  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  allocInfo.requiredFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  m_instanceBuffer = GraphicsBuffer(m_allocator, bufferInfo, allocInfo);

  m_maxInstances = maxInstances;

  return true;
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

bool TileRender::setTexture(size_t index, const std::string& path) {
  if(!isValidImageFile(path)) {
    return false;
  }
  
  u8* srcData;
  int width, height, channels;
  stbi_set_flip_vertically_on_load(true);
  srcData = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha); 
  if(!srcData) {
    return false;
  }

  void* dstData;
  vmaMapMemory(m_allocator, m_stagingImage, &dstData);
  std::memcpy(dstData, srcData, width * height * channels);
  vmaUnmapMemory(m_allocator, m_stagingImage);
  vmaFlushAllocation(m_allocator, m_stagingImage, 0, width * height * channels);

  vk::FenceCreateInfo fenceCreateInfo;
  vk::Fence fence = m_device.createFence(fenceCreateInfo);

  vk::CommandBufferAllocateInfo cmdAllocInfo;
  cmdAllocInfo.commandBufferCount = 1;
  cmdAllocInfo.commandPool = m_utilityCmdPool;
  cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;
  vk::CommandBuffer cmdBuffer = m_device.allocateCommandBuffers(cmdAllocInfo)[0];

  vk::CommandBufferBeginInfo beginInfo;
  cmdBuffer.begin(beginInfo);

  // Transition staging image to TRANSFER_SRC
  vk::ImageMemoryBarrier barrier{};
  barrier.oldLayout = vk::ImageLayout::eUndefined;
  barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = m_stagingImage;
  barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
  cmdBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTopOfPipe,
      vk::PipelineStageFlagBits::eTransfer,
      {}, {}, {}, barrier);

  barrier.oldLayout = vk::ImageLayout::eUndefined;
  barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.image = m_tileTextures;
  barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, index, 1);
  cmdBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eFragmentShader,
      vk::PipelineStageFlagBits::eTransfer,
    {}, {}, {}, barrier);

  // coopy
  std::vector<vk::ImageCopy> imageCopies(1);
  imageCopies[0].srcOffset = vk::Offset3D(0, 0, 0);
  imageCopies[0].dstOffset = vk::Offset3D(0, 0, 0);
  imageCopies[0].extent = vk::Extent3D(tileTexturePixelSize.x, tileTexturePixelSize.y, 1);
  imageCopies[0].srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  imageCopies[0].srcSubresource.baseArrayLayer = 0;
  imageCopies[0].srcSubresource.layerCount = 1;
  imageCopies[0].srcSubresource.mipLevel = 0;
  imageCopies[0].dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  imageCopies[0].dstSubresource.baseArrayLayer = index;
  imageCopies[0].dstSubresource.layerCount = 1;
  imageCopies[0].dstSubresource.mipLevel = 0;
  cmdBuffer.copyImage((vk::Image)m_stagingImage, vk::ImageLayout::eTransferSrcOptimal, 
                      (vk::Image)m_tileTextures, vk::ImageLayout::eTransferDstOptimal, imageCopies);

  // Transition tile array to eShaderReadOnlyOptimal (you usually want to do *only* the layer you're copying)
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.image = m_tileTextures;
  barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, index, 1);
  cmdBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eFragmentShader,
    {}, {}, {}, barrier);

  cmdBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;
  m_utilityQueue.submit(submitInfo, fence);

  std::vector<vk::Fence> waitFences = { fence };
  m_device.waitForFences(waitFences, true, UINT64_MAX);
  m_device.resetFences(waitFences);
  m_device.destroyFence(fence);

  return true;
}

RAMPAGE_END