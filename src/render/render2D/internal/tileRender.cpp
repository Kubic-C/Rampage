#include "tileRender.hpp"

RAMPAGE_START

TileRender::TileRender(VulkanContext& context)
  : VulkanRender(context) {}

void TileRender::reset() {

}

void TileRender::process(const std::vector<DrawTileCmd>& cmds) {

}

void TileRender::draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) {

}

bool TileRender::recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) { 
  return true;
}

RAMPAGE_END