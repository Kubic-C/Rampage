#include "context.hpp"

RAMPAGE_START

VulkanContext::VulkanContext(IHost& host, SDL_Window* window, bool enableValidationLayers)
  : m_host(host), m_window(window) {
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


  vk::SemaphoreCreateInfo semaphoreInfo;
  swapchain->imageAvailableSemaphore = m_device.createSemaphore(semaphoreInfo);

  return swapchain;
}

std::shared_ptr<SwapchainRenderTargets> VulkanContext::createSwapchainRenderTargets(const std::shared_ptr<Swapchain>& swapchain, const std::shared_ptr<SwapchainRenderTargets>& previous) {
  std::shared_ptr<SwapchainRenderTargets> renderTargets = std::make_unique<SwapchainRenderTargets>();
  renderTargets->extent = swapchain->extent;
  renderTargets->allocator = m_allocator;
  renderTargets->device = m_device;
  renderTargets->pool = m_commandPool;

  // RENDER PASS
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
  vk::AttachmentReference colorRef{};
  colorRef.attachment = 0; // index in attachment array
  colorRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference depthRef{};
  depthRef.attachment = 1;
  depthRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;
  std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
  vk::RenderPassCreateInfo renderPassInfo{};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderTargets->mainRenderPass = m_device.createRenderPass(renderPassInfo);
  if(!renderTargets->mainRenderPass)
    return nullptr;

  // FRAMES
  renderTargets->frames.resize(swapchain->images.size());
  for(size_t i = 0; i < renderTargets->frames.size(); i++) {
    auto& frame = renderTargets->frames[i];
    auto& colorImage = swapchain->images[i];

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = colorImage;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = swapchain->imageFormat;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    frame.colorImageView = m_device.createImageView(viewInfo);

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
    VkImage depthImage;
    VmaAllocation depthImageAlloc;
    VkResult result = vmaCreateImage(m_allocator, depthImageInfo, &allocInfo, &depthImage, &depthImageAlloc, nullptr);

    frame.depthImageMemory = depthImageAlloc;
    frame.depthImage = depthImage;

    // Create image view for depth
    vk::ImageViewCreateInfo depthViewInfo{};
    depthViewInfo.image = frame.depthImage;
    depthViewInfo.viewType = vk::ImageViewType::e2D;
    depthViewInfo.format = vk::Format::eD32SfloatS8Uint;
    depthViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;
    frame.depthImageView = m_device.createImageView(depthViewInfo);

    if(!frame.depthImageMemory || 
      !frame.depthImage || 
      !frame.depthImageView) {
        return nullptr;
    }


    std::array<vk::ImageView, 2> attachments = {
      frame.colorImageView,  // color
      frame.depthImageView // depth
    };

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderTargets->mainRenderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = swapchain->extent.width;
    fbInfo.height = swapchain->extent.height;
    fbInfo.layers = 1;
    frame.framebuffer = m_device.createFramebuffer(fbInfo);

    vk::CommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.commandPool = m_commandPool;
    cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdAllocInfo.commandBufferCount = 1;
    frame.cmdBuffer = m_device.allocateCommandBuffers(cmdAllocInfo)[0];

    vk::FenceCreateInfo fenceInfo;
    frame.fence = m_device.createFence(fenceInfo);

    vk::SemaphoreCreateInfo semaphoreInfo;
    frame.renderFinishedSemaphore = m_device.createSemaphore(semaphoreInfo);
  }

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