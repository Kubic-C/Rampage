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
public:
  TileRender(VulkanContext& context);
  void reset();
  void process(const std::vector<DrawTileCmd>& cmds);
  void draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) override;

  bool recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) override;

  virtual std::vector<NeededDescriptorType> getNeededDescriptorTypes() const override {
    return {
      {vk::DescriptorType::eUniformBuffer, 1}
    };
  }

  virtual void createDescriptorSets(vk::DescriptorPool pool) override {
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

    vk::DescriptorSetAllocateInfo allocInfoDS{};
    allocInfoDS.descriptorPool = pool;
    allocInfoDS.descriptorSetCount = 1;
    allocInfoDS.pSetLayouts = &m_descSetLayout;
    m_descSet = m_device.allocateDescriptorSets(allocInfoDS)[0];

    // This sets the pointer in the descriptor set
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

private:
  // Uniforms.
  vk::DescriptorSet m_descSet = {};
  vk::DescriptorSetLayout m_descSetLayout;
  GraphicsBuffer m_vpBuffer, m_tileSideLengthBuffer, m_samplerBuffer;
  // Buffers
  GraphicsBuffer indexBuffer, vertexBuffer, instanceBuffer;
  // Pipeline
  vk::PipelineLayout m_pipelineLayout;
  vk::Pipeline m_pipeline;
};

RAMPAGE_END