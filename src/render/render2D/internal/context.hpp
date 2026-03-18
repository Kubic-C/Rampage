#pragma once

#include "vkcommon.hpp"

RAMPAGE_START

class VulkanContext;

struct Swapchain {
  ~Swapchain() {
    if(swapchain)
      device.destroySwapchainKHR(swapchain);
    if(m_imageAvailableSemaphore)
      device.destroy(m_imageAvailableSemaphore);
    if(m_renderFinishedSemaphore)
      device.destroy(m_renderFinishedSemaphore);
  }

  vk::Extent2D extent;
  vk::SwapchainKHR swapchain;
  vk::Format imageFormat;
  std::vector<vk::Image> images;
  vk::Semaphore m_imageAvailableSemaphore;
  vk::Semaphore m_renderFinishedSemaphore;

  vk::Device device;
};

struct SwapchainRenderTargets {
  struct Frame {
    vk::ImageView colorImageView;
    vk::Image depthImage;
    vk::ImageView depthImageView;
    VmaAllocation depthImageMemory;
    vk::Framebuffer framebuffer;
    vk::CommandBuffer cmdBuffer;
    vk::Fence fence;
  };
  
  ~SwapchainRenderTargets() {
    if(mainRenderPass)
      device.destroyRenderPass(mainRenderPass);
    for(auto& frame : frames) {
      if(frame.colorImageView)
        device.destroyImageView(frame.colorImageView);
      if(frame.depthImage)
        device.destroyImage(frame.depthImage);
      if(frame.depthImageView)
        device.destroyImageView(frame.depthImageView);
      vmaFreeMemoryPages(allocator, 1, &frame.depthImageMemory);
      if(frame.framebuffer)
        device.destroyFramebuffer(frame.framebuffer);
      device.freeCommandBuffers(pool, {frame.cmdBuffer});
      if(frame.fence)
        device.destroyFence(frame.fence);
    }
  }

  vk::Extent2D extent;
  std::vector<Frame> frames;
  vk::RenderPass mainRenderPass;

  VmaAllocator allocator;
  vk::CommandPool pool;
  vk::Device device;
};

class VulkanContext {
public:
  std::vector<const char*> neededValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  std::vector<const char*> neededExtensions = {};

  std::vector<const char*> neededDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  std::array<vk::PresentModeKHR, 3> neededPresentModes = {
    vk::PresentModeKHR::eFifo,
    vk::PresentModeKHR::eMailbox,
    vk::PresentModeKHR::eImmediate
  };

  vk::Format neededSurfaceFormat = vk::Format::eR8G8B8A8Srgb;
  vk::ColorSpaceKHR neededColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

  struct SelectedPhysicalDeviceInfo {
    u32 deviceIdx = std::numeric_limits<u32>::max();
    u32 graphicsQueueIdx = std::numeric_limits<u32>::max();
    u32 presentQueueIdx = std::numeric_limits<u32>::max();
    vk::SurfaceFormatKHR surfaceFormat;
    vk::PhysicalDeviceMemoryProperties memoryProps;
  };

  VulkanContext(SDL_Window* window, bool enableValidationLayers);
  ~VulkanContext();

  Status getStatus() const {
    return m_status;
  }

  vk::Device& getDevice() {
    return m_device;
  }

  VmaAllocator getAllocator() {
    return m_allocator;
  }

  vk::Queue& getGraphicsQueue() {
    return m_graphicsQueue;
  }

  vk::Queue& getPresentQueue() {
    return m_presentQueue;
  }

  std::shared_ptr<Swapchain> createSwapchain(u32 width, u32 height, vk::PresentModeKHR presentMode);
  std::shared_ptr<SwapchainRenderTargets> createSwapchainRenderTargets(const std::shared_ptr<Swapchain>& swapchain, const std::shared_ptr<SwapchainRenderTargets>& previous);

private:
  bool isSurfacePresentModesSupported(const std::vector<vk::PresentModeKHR>& availablePresentModes);
  bool isValidationLayersSupported();
  bool isExtensionsSupported();
  SelectedPhysicalDeviceInfo findSuitablePhysicalDevice();

  void markCriticalError() {
    m_status = Status::CriticalError;
  }

  Status m_status = Status::Ok;
  SDL_Window* m_window;
  vk::Instance m_instance;
  VkSurfaceKHR m_surface;
  SelectedPhysicalDeviceInfo m_selDevice;
  vk::PhysicalDevice m_physicalDevice;
  vk::Device m_device;
  vk::Queue m_graphicsQueue;
  vk::Queue m_presentQueue;
  VmaAllocator m_allocator;
  vk::CommandPool m_commandPool;
};

inline vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char>& code) {
  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  return device.createShaderModule(createInfo);
}

// Reads a binary file and returns a vk::UniqueShaderModule
inline vk::ShaderModule loadShaderModuleFromFile(vk::Device device, const std::string& filename) {
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

RAMPAGE_END