#pragma once

#include "context.hpp"
#include "graphicsPipelineBuilder.hpp"

RAMPAGE_START

struct NeededDescriptorType {
  vk::DescriptorType type = static_cast<vk::DescriptorType>(0);
  u32 count = 0;
};

class VulkanRender {
public:
  VulkanRender(VulkanContext& context)
    : m_host(context.getHost()), m_allocator(context.getAllocator()), m_device(context.getDevice()), m_utilityQueue(context.getGraphicsQueue()), m_utilityCmdPool(context.getCommandPool()) {}

  static constexpr u32 maxSetsPerRenderRender = 5;

  virtual void createDescriptorSets(vk::DescriptorPool pool) = 0;
  virtual std::vector<NeededDescriptorType> getNeededDescriptorTypes() const = 0;
  virtual void draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) = 0;
  virtual bool recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) = 0;

protected:
  IHost& m_host;
  VmaAllocator m_allocator;
  vk::Device m_device;
  vk::Queue m_utilityQueue;
  vk::CommandPool m_utilityCmdPool;
};

inline vk::DescriptorPool createDescriptorPoolFromRenders(VulkanContext& context, const std::vector<std::shared_ptr<VulkanRender>> renders) {
  vk::Device device = context.getDevice();
  std::vector<vk::DescriptorPoolSize> poolSizes;
  OpenMap<vk::DescriptorType, u32> typeCounts;

  for(const auto& render : renders) {
    for(NeededDescriptorType renderTypeCount : render->getNeededDescriptorTypes()) {
      if(!typeCounts.contains(renderTypeCount.type))
        typeCounts[renderTypeCount.type] = renderTypeCount.count;
      else
        typeCounts[renderTypeCount.type] += renderTypeCount.count;
    }
  }

  for(auto& [type, count] : typeCounts) {
    auto& poolSize = poolSizes.emplace_back();
    poolSize.type = type;
    poolSize.descriptorCount = count;
  }

  vk::DescriptorPoolCreateInfo poolCreateInfo;
  poolCreateInfo.maxSets = (u32)renders.size() * VulkanRender::maxSetsPerRenderRender; 
  poolCreateInfo.poolSizeCount = (u32)poolSizes.size();
  poolCreateInfo.pPoolSizes = poolSizes.data();
  auto result = device.createDescriptorPool(poolCreateInfo);
  if(!result) {
    return nullptr;
  } else {
    return result;
  }
}

RAMPAGE_END