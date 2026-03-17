#pragma once

#define VULKAN_HPP_NO_SMART_HANDLE 1
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS 1

#include "commands.hpp"
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_structs.hpp>
#include <vk_mem_alloc.h>

/* SHOULD NOT BE INCLUDED IN RENDER2D.HPP */

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

  u32 nextImage(VulkanContext& context);

  vk::Device device;
};

struct SwapchainRenderTargets {
  ~SwapchainRenderTargets() {
    if(mainRenderPass)
      device.destroyRenderPass(mainRenderPass);
    for(auto framebuffer : framebuffers) {
      if(framebuffer)
        device.destroyFramebuffer(framebuffer);
    }
    for(auto view : imageViews) {
      if(view)
        device.destroyImageView(view);
    }
    for(auto view : depthImageViews) {
      if(view)
        device.destroyImageView(view);
    }
    for(auto image : depthImages) {
      if(image)
        device.destroyImage(image);
    }
    for(auto fence : frameFences) {
      if(fence)
        device.destroyFence(fence);
    }
    device.freeCommandBuffers(pool, commandBuffers);
    vmaFreeMemoryPages(allocator, depthImageMemories.size(), depthImageMemories.data());
  }

  vk::Extent2D extent;
  std::vector<vk::ImageView> imageViews;
  std::vector<vk::Image> depthImages;
  std::vector<vk::ImageView> depthImageViews;
  std::vector<VmaAllocation> depthImageMemories;
  std::vector<vk::Framebuffer> framebuffers;
  std::vector<vk::CommandBuffer> commandBuffers;
  std::vector<vk::Fence> frameFences;
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

struct DrawInfo {
  vk::CommandBuffer& cmdBuffer;
  vk::Framebuffer& framebuffer;
  vk::RenderPass& mainRenderPass;
  vk::Extent2D extent;
  glm::mat4 vp;
};

class ShapeRender {
  struct ShapeVertex {
    glm::vec3 pos;
    glm::vec3 color;
  };

public:
  ShapeRender(VulkanContext& context);
  ~ShapeRender();

  void reset();
  void process(const std::vector<DrawRectangleCmd>& cmds);
  void process(const std::vector<DrawCircleCmd>& cmds);
  void process(const std::vector<DrawLineCmd>& cmds);
  void process(const std::vector<DrawHollowCircleCmd>& cmds);
  void draw(DrawInfo& drawInfo);

  bool createVertexBuffer(size_t maxVerticesCount);
  bool createPipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets);

private:
  VulkanContext& m_context;
  // Uniforms.
  vk::DescriptorPool m_descPool;
  std::vector<vk::DescriptorSet> m_descSets = {};
  vk::DescriptorSetLayout m_descSetLayout;
  VkBuffer m_uvpBuffer = nullptr;
  VmaAllocation m_uvpAllocation = nullptr; 
  // Pipeline
  vk::PipelineLayout m_pipelineLayout;
  vk::Pipeline m_pipeline;
  // Vulkan Buffers
  VkBuffer m_vertexBuffer = nullptr;
  VmaAllocation m_vertexAllocation = nullptr;
  size_t m_maxVerticesCount = 0;
  // CPU-Side Vertices
  std::vector<ShapeVertex> m_vertices;
  std::vector<ShapeVertex> m_baseCircleMesh;

  void generateCircleMesh(int resolution);
  void addLine(const DrawLineCmd& cmd);
};

class TileRender {
public:
  TileRender();
  void reset();
  void process(const std::vector<DrawTileCmd>& cmds);
  void draw();

private:
  
};

class LightRender {
public:
  LightRender();
  void reset();
  void process(const std::vector<DrawLightCmd>& cmds);
  void draw();

private:
};

class InternalRender2D {
public:
  InternalRender2D(SDL_Window* window, bool enableValidationLayers) 
    : m_window(window) {
    m_context = std::make_unique<VulkanContext>(window, enableValidationLayers);
    if(m_context->getStatus() != Status::Ok) {
      markCriticalError();
      return;
    }

    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    reset({width, height});
  }

  Status getStatus() const {
    return m_status;
  }

  std::unique_ptr<ShapeRender> shapeRender;
  TileRender tileRender;
  LightRender lightRender;

  void reset(const glm::ivec2& size) {
    m_swapchain = nullptr;
    m_renderTargets = nullptr;
    shapeRender = nullptr;

    m_swapchain = m_context->createSwapchain((u32)size.x, (u32)size.y, vk::PresentModeKHR::eFifo);
    if(!m_swapchain) {
      markCriticalError();
      return;
    }

    m_renderTargets = m_context->createSwapchainRenderTargets(m_swapchain, nullptr);
    if(!m_renderTargets) {
      markCriticalError();
      return;
    }

    shapeRender = std::make_unique<ShapeRender>(*m_context);
    if(!shapeRender->createPipeline(m_renderTargets)) {
      markCriticalError();
      return;
    }
  }

  void draw(const glm::mat4& vp) {
    u32 imageIndex = m_swapchain->nextImage(*m_context);

    DrawInfo info = {
      m_renderTargets->commandBuffers[imageIndex],
      m_renderTargets->framebuffers[imageIndex],
      m_renderTargets->mainRenderPass,
      m_renderTargets->extent,
      vp,
    };

    shapeRender->draw(info);

    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_swapchain->m_imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_renderTargets->commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_swapchain->m_renderFinishedSemaphore;
    
    m_context->getGraphicsQueue().submit(submitInfo, m_renderTargets->frameFences[imageIndex]);
    
    vk::PresentInfoKHR presentInfo{};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain->swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_swapchain->m_renderFinishedSemaphore;
    vk::Result result = m_context->getPresentQueue().presentKHR(presentInfo);
    if(result == vk::Result::eErrorOutOfDateKHR) {
      int width, height;
      SDL_GetWindowSize(m_window, &width, &height);
      reset({width, height});
    }

    std::vector<vk::Fence> fences = {vk::Fence(m_renderTargets->frameFences[imageIndex])};
    m_context->getDevice().waitForFences(fences, true, UINT64_MAX);
    m_context->getDevice().resetFences(fences);
  }
  
  private:
  void markCriticalError() {
    m_status = Status::CriticalError;
  }

  Status m_status = Status::Ok;
  SDL_Window* m_window;
  std::shared_ptr<VulkanContext> m_context;
  std::shared_ptr<Swapchain> m_swapchain;
  std::shared_ptr<SwapchainRenderTargets> m_renderTargets;
};

RAMPAGE_END