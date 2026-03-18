#pragma once

#include "../commands.hpp"
#include "render.hpp"

RAMPAGE_START

static constexpr const char* TILE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/tilevs.spv";
static constexpr const char* TILE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/tilefs.spv";

class TileRender : public VulkanRender {
public:
  TileRender(VulkanContext& context);
  void reset();
  void process(const std::vector<DrawTileCmd>& cmds);
  void draw(vk::CommandBuffer& cmdBuffer, const glm::mat4& vp) override;

  bool recreatePipeline(const std::shared_ptr<SwapchainRenderTargets>& renderTargets) override;

  virtual std::vector<NeededDescriptorType> getNeededDescriptorTypes() const override {
    return {};
  }

  virtual void createDescriptorSets(vk::DescriptorPool pool) override {}

private:
  
};

RAMPAGE_END