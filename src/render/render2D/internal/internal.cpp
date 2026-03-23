#include "internal.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

RAMPAGE_START

InternalRender2D::InternalRender2D(IHost& host, SDL_Window* window, bool enableValidationLayers) 
  : m_window(window) {
  m_context = std::make_unique<VulkanContext>(host, window, enableValidationLayers);
  if(m_context->getStatus() != Status::Ok) {
    markCriticalError();
    return;
  }

  int width, height;
  SDL_GetWindowSize(m_window, &width, &height);

  shapeRender = std::make_shared<ShapeRender>(*m_context);
  tileRender  = std::make_shared<TileRender>(*m_context);
  m_renders.push_back(shapeRender);
  m_renders.push_back(tileRender);

  m_descPool = createDescriptorPoolFromRenders(*m_context, m_renders);
  for(const auto& render : m_renders) {
    render->createDescriptorSets(m_descPool);
  }

  reset({width, height});
}

Status InternalRender2D::getStatus() const {
  return m_status;
}

void InternalRender2D::reset(const glm::ivec2& size) {
  auto device = m_context->getDevice();

  m_swapchain = nullptr;
  m_renderTargets = nullptr;

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

  for(auto& render : m_renders) {
    if(!render->recreatePipeline(m_renderTargets)) {
      markCriticalError();
      return;
    }
  }
}

void InternalRender2D::draw(const glm::mat4& vp, const glm::vec4& clearRGBA) {
  auto device = m_context->getDevice();

  // Target Selection
  u32 imageIndex;
  vk::Result acquireResult = device.acquireNextImageKHR(m_swapchain->swapchain, UINT64_MAX, m_swapchain->imageAvailableSemaphore, nullptr, &imageIndex);
  if(acquireResult != vk::Result::eSuccess) {
    throw std::runtime_error("Coudln't acquire image!");
  }
  auto& frame = m_renderTargets->frames[imageIndex];
  vk::RenderPass renderPass = m_renderTargets->mainRenderPass;
  vk::Extent2D drawArea = m_renderTargets->extent;

  // Drawing
  frame.cmdBuffer.reset();
  vk::CommandBufferBeginInfo beginInfo{};
  frame.cmdBuffer.begin(beginInfo);
  vk::ClearValue clearValues[2];
  clearValues[0].color = vk::ClearColorValue(clearRGBA.r, clearRGBA.g, clearRGBA.b, clearRGBA.a);
  clearValues[1].depthStencil.depth = 1.0f;
  clearValues[1].depthStencil.stencil = 0;
  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = frame.framebuffer;
  renderPassInfo.renderArea.extent = drawArea;
  renderPassInfo.clearValueCount = 2;
  renderPassInfo.pClearValues = clearValues;
  frame.cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  for(const auto& render : m_renders) {
    render->draw(frame.cmdBuffer, vp);
  }

  frame.cmdBuffer.endRenderPass();
  frame.cmdBuffer.end();

  // Submit
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput
  };
  vk::SubmitInfo submitInfo{};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &m_swapchain->imageAvailableSemaphore;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &frame.cmdBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderTargets->frames[imageIndex].renderFinishedSemaphore;

  m_context->getGraphicsQueue().submit(submitInfo, frame.fence);
  
  std::vector<vk::Fence> fences = {frame.fence};
  m_context->getDevice().waitForFences(fences, true, UINT64_MAX);
  m_context->getDevice().resetFences(fences);

  // Swap buffers
  vk::PresentInfoKHR presentInfo{};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapchain->swapchain;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &m_renderTargets->frames[imageIndex].renderFinishedSemaphore;
  vk::Result result = m_context->getPresentQueue().presentKHR(presentInfo);
  if(result == vk::Result::eErrorOutOfDateKHR) {
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);
    reset({width, height});
  }
}

RAMPAGE_END