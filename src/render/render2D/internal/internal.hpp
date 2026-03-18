#pragma once

#include "shapeRender.hpp"
#include "tileRender.hpp"

/* SHOULD NOT BE INCLUDED IN RENDER2D.HPP */

RAMPAGE_START

class InternalRender2D {
public:
  InternalRender2D(SDL_Window* window, bool enableValidationLayers);

  Status getStatus() const;
  void reset(const glm::ivec2& size);
  void draw(const glm::mat4& vp, const glm::vec4& clearRGBA = {0, 0, 0, 1});
  
public:
  std::shared_ptr<ShapeRender> shapeRender;
  std::shared_ptr<TileRender> tileRender;

private:
  void markCriticalError() {
    m_status = Status::CriticalError;
  }

  Status m_status = Status::Ok;
  SDL_Window* m_window;
  std::shared_ptr<VulkanContext> m_context;
  std::shared_ptr<Swapchain> m_swapchain;
  std::shared_ptr<SwapchainRenderTargets> m_renderTargets;
  std::vector<std::shared_ptr<VulkanRender>> m_renders;
  vk::DescriptorPool m_descPool;
};

RAMPAGE_END