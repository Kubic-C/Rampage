#pragma once

#include "../commands.hpp"
#include "render.hpp"

RAMPAGE_START

static constexpr const char* TILE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/tilevs.spv";
static constexpr const char* TILE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/tilefs.spv";

class TileRender : public VulkanRender {
  struct TileVertex {
    glm::vec2 pos;
    glm::vec2 texCoords;
  };

  struct TileInstance {
    static constexpr u8 zMask = 0b00000111; // 7
    static constexpr u8 rotMask = 0b11111000; // 248 (31)
    static constexpr u8 dimXMask = 0b00001111; // 15
    static constexpr u8 dimYMask = 0b11110000; // 240 (15)

    glm::vec2 localPos;
    glm::vec2 worldPos;
    u16 layer;
    float rot;
    float z;
    u8 dim;
    float scale;

    void setDim(u8 dimX, u8 dimY) {
      constexpr u8 rangeLimit = 15;
      assert(dimX <= rangeLimit);
      assert(dimY <= rangeLimit);

      dim = 0;
      dim |= dimX;
      dim |= dimY << 4;
    }
  };

  static constexpr u32 maxTileTextures = 256;
  static constexpr glm::ivec2 tileTexturePixelSize = glm::ivec2(32, 32);
public:
  TileRender(VulkanContext& context);
  void reset();
  void process(const std::vector<DrawTileCmd>& cmds);
  void draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) override;

  bool createInstanceBuffer(size_t maxInstances);
  bool recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) override;

  virtual std::vector<NeededDescriptorType> getNeededDescriptorTypes() const override {
    return {
      {vk::DescriptorType::eUniformBuffer, 2},
      {vk::DescriptorType::eCombinedImageSampler, 1}
    };
  }

  virtual void createDescriptorSets(vk::DescriptorPool pool) override {
    vk::DescriptorSetAllocateInfo allocInfoDS{};
    allocInfoDS.descriptorPool = pool;
    allocInfoDS.descriptorSetCount = 1;
    allocInfoDS.pSetLayouts = &m_descSetLayout;
    m_descSet = m_device.allocateDescriptorSets(allocInfoDS)[0];

    // This sets the pointer in the descriptor set
    {
      vk::DescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = m_vpBuffer;
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
    {
      vk::DescriptorBufferInfo bufferInfo{};
      bufferInfo.buffer = m_tileSideLengthBuffer;
      bufferInfo.offset = 0;
      bufferInfo.range = sizeof(float);
      vk::WriteDescriptorSet descriptorWrite{};
      descriptorWrite.dstSet = m_descSet;
      descriptorWrite.dstBinding = 1;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pBufferInfo = &bufferInfo;
      m_device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
    }
    {
      vk::DescriptorImageInfo imageInfo{};
      imageInfo.sampler = m_tileTextureSampler;
      imageInfo.imageView = m_tileTexturesView;
      imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      vk::WriteDescriptorSet descriptorWrite{};
      descriptorWrite.dstSet = m_descSet;
      descriptorWrite.dstBinding = 2;
      descriptorWrite.dstArrayElement = 0;
      descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
      descriptorWrite.descriptorCount = 1;
      descriptorWrite.pImageInfo = &imageInfo;
      m_device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
    }
  }

  inline bool isValidImageFile(const std::string& path) {
    int x, y, n;
    int ok = stbi_info(path.c_str(), &x, &y, &n);
    return ok != 0 && glm::ivec2(x, y) == tileTexturePixelSize;
  }

  TileTextureId loadSprite(const std::string& path) {
    std::string name = getFilename(path);
    if(textureIds.contains(name))
      return textureIds[name];

    TileTextureId id = ++lastFreeTexture;
    if (!setTexture(id.value(), path)) {
      lastFreeTexture--;
      return TileTextureId::null();
    }
    textureIds[name] = id;
    texturePaths.resize(id.value() + 1);
    texturePaths[id.value()] = name;

    return id;
  }

  TileTextureId getSprite(const std::string& name) {
    assert(getFilename(name) == name && "When using getSprite, use only the core filename");
    return textureIds.find(name)->second;
  }
  
  std::string getSpritePath(TileTextureId id) {
    return texturePaths[id.value()];
  }

private:
  TileTextureId lastFreeTexture = TileTextureId::null();
  Map<std::string, TileTextureId> textureIds;
  std::vector<std::string> texturePaths;
  bool setTexture(size_t index, const std::string& path);

  // Uniforms.
  vk::DescriptorSet m_descSet = {};
  vk::DescriptorSetLayout m_descSetLayout;
  GraphicsBuffer m_vpBuffer, m_tileSideLengthBuffer;
  // Buffers
  size_t m_maxInstances = 0;
  std::vector<TileInstance> m_tileInstances;
  GraphicsBuffer m_indexBuffer, m_vertexBuffer, m_instanceBuffer;
  // Pipeline
  vk::PipelineLayout m_pipelineLayout;
  vk::Pipeline m_pipeline;

  // Texture loader
  GraphicsImage m_tileTextures;
  VkImageView m_tileTexturesView;
  vk::Sampler m_tileTextureSampler;
  GraphicsImage m_stagingImage;
};

RAMPAGE_END