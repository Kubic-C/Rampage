#pragma once

#include "../common/common.hpp"
#include "stb_image.h"

RAMPAGE_START


struct InstanceBufferComponent {
  size_t capacity = 0;
  size_t count = 0;
  void* instances = nullptr; // Pointer to mapped Instance Buffer
  bgfx::VertexBufferHandle indexBuffer; // Per-Instance Element/Indicies
  bgfx::VertexBufferHandle meshBuffer; // Per-Instance Mesh
  bgfx::VertexBufferHandle instanceBuffer; // List of Instances
};

struct TextureMap3DComponent {
  u32 lastFreeTexture = 0;
  Map<std::string, u32> textureIds;
  const u16 width = 16;
  const u16 height = 16;
  bgfx::TextureHandle texArray;
  
  TextureMap3DComponent() {
    texArray = bgfx::createTexture3D(width, height, 256, true, bgfx::TextureFormat::RGBA32F, 
      BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_UVW_MIRROR);

    // Create a 32x32 checkerboard of black and purple
    const int size = 32;
    const int boxSize = 16;  // 16x16 pixel boxes
    const int channels = 4; // RGBA
    const bgfx::Memory* mem = bgfx::alloc(size * size * channels);
    
    // Fill with checkerboard pattern
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        int boxX = x / boxSize;
        int boxY = y / boxSize;
        bool isRed = (boxX + boxY) % 2 == 0;
        
        int idx = (y * size + x) * channels;
        if (isRed) {
          mem->data[idx + 0] = 255;     // R - Red
          mem->data[idx + 1] = 0;     // G
          mem->data[idx + 2] = 0;     // B
          mem->data[idx + 3] = 255;   // A
        } else {
          mem->data[idx + 0] = 128;   // R - Purple
          mem->data[idx + 1] = 0;     // G
          mem->data[idx + 2] = 128;   // B
          mem->data[idx + 3] = 255;   // A
        }
      }
    }

    bgfx::updateTexture3D(texArray, 0, 0, 0, 0, size, size, 1, mem);
    
    textureIds["unknown"] = 0;
  }

  inline bool isValidImageFile(const std::string& path) {
    int x, y, n;
    int ok = stbi_info(path.c_str(), &x, &y, &n);
    return ok != 0 && x == width && y == height;
  }

  u32 loadSprite(const std::string& path) {
    std::string name = getFilename(path);
    if(textureIds.contains(name))
      return false;

    u32 id = ++lastFreeTexture;
    if (!loadSprite(id, path)) {
      lastFreeTexture--;
      return UINT32_MAX;
    }
    textureIds[name] = id;

    return id;
  }

  u32 getSprite(const std::string& name) {
    assert(getFilename(name) == name && "When using getSprite, use only the core filename");
    return textureIds.find(name)->second;
  }

  bool loadSprite(uint32_t index, std::string path) {
    assert(index < 256);

    int width, height, channels;
    u8* data;
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
      return false;

    const bgfx::Memory* mem = bgfx::copy(data, width * height * channels);
    bgfx::updateTexture3D(texArray, 0, 0, 0, index, width, height, 1, mem);
    stbi_image_free(data);

    return true;
  }
};

struct TextureMapInUseTag {};

RAMPAGE_END
