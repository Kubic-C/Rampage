#pragma once

#include "opengl/opengl.hpp"

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

  TextureMap3DComponent()
    : texArray(32, 32, 256) {
    sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  u32 loadSprite(const std::string& path) {
    std::string name = getFilename(path);
    assert(!textureIds.contains(name));

    u32 id = lastFreeTexture++;
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

  // Gets the sprite located at path, if the sprite with the core name does not
  // exist yet it is created. if the core name already exists, that sprite will
  // be grabbed instead.
  u32 getSpriteFromPath(const std::string& path) {
    std::string name = getFilename(path);
    if (textureIds.contains(name))
      return getSprite(name);

    return loadSprite(path);
  }

  bool loadSprite(uint32_t index, const std::string& path) {
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

struct TextureMapInUseTag {
  EntityId id;
};

RAMPAGE_END