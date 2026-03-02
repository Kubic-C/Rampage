#pragma once

#include "opengl/opengl.hpp"
#include <filesystem>

RAMPAGE_START

using VertexArrayBufferComponent = VertexArrayBuffer;

using ShaderComponent = Shader;

template <typename VertexType, size_t verticePerRender, size_t verticesPerSubdata>
using MeshBufferComponent = Mesh<VertexType, verticePerRender, verticesPerSubdata>;

struct InstanceBufferComponent {
  size_t capacity = 0;
  size_t count = 0;
  void* instances = nullptr; // Pointer to mapped Instance Buffer
  VertexBuffer indexBuffer; // Per-Instance Element/Indicies
  VertexBuffer meshBuffer; // Per-Instance Mesh
  VertexBuffer instanceBuffer; // List of Instances
};

struct TextureMap3DComponent {
  u32 lastFreeTexture = 0;
  Map<std::string, u32> textureIds;
  TextureArray texArray;
  Sampler sampler;

  void initDefaultTexture() {
    // Create a 32x32 checkerboard of black and purple
    const int size = 32;
    const int boxSize = 16;  // 16x16 pixel boxes
    const int channels = 4; // RGBA
    u8* data = new u8[size * size * channels];
    
    // Fill with checkerboard pattern
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        int boxX = x / boxSize;
        int boxY = y / boxSize;
        bool isRed = (boxX + boxY) % 2 == 0;
        
        int idx = (y * size + x) * channels;
        if (isRed) {
          data[idx + 0] = 255;     // R - Red
          data[idx + 1] = 0;     // G
          data[idx + 2] = 0;     // B
          data[idx + 3] = 255;   // A
        } else {
          data[idx + 0] = 128;   // R - Purple
          data[idx + 1] = 0;     // G
          data[idx + 2] = 128;   // B
          data[idx + 3] = 255;   // A
        }
      }
    }
    
    texArray.subWholeImage(0, data);
    delete[] data;
    
    textureIds["unknown"] = 0;
  }

  TextureMap3DComponent() : texArray(32, 32, 1024) {
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

    initDefaultTexture();
  }

  inline bool isValidImageFile(const std::string& path) {
    int x, y, n;
    int ok = stbi_info(path.c_str(), &x, &y, &n);
    return ok != 0 && x == texArray.getWidth() && y == texArray.getHeight();
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

    texArray.subWholeImage(index, data);
    stbi_image_free(data);

    return true;
  }
};

struct TextureMapInUseTag {};

RAMPAGE_END
