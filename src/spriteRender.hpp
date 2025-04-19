#pragma once

#include "render/shapes.hpp"
#include "transform.hpp"
#include "tilemap.hpp"
#include "seekPlayer.hpp"

struct SpriteLayer {
  SpriteLayer() = default;
  SpriteLayer(u16 texIndex, glm::vec2 offset, float rot)
    : texIndex(texIndex), offset(offset), rot(rot) {
  }

  u16 texIndex = 0;
  glm::vec2 offset = { 0.0f, 0.0f };
  float rot = 0.0f;
};

struct SpriteComponent {
  static constexpr size_t MAX_SPRITE_LAYERS = 4;
  SpriteLayer layers[MAX_SPRITE_LAYERS];
  float zOffset = -1.0f;
  u8 layerCount = 0;

  void addLayer(const SpriteLayer& layer) {
    assert(layerCount < MAX_SPRITE_LAYERS && "Too many sprite layers!");
    layers[layerCount++] = layer;
  }

  void addLayer(u32 texIndex, glm::vec2 offset = Vec2(0), float rot = 0) {
    assert(layerCount < MAX_SPRITE_LAYERS && "Too many sprite layers!");
    layers[layerCount++] = SpriteLayer(texIndex, offset, rot);
  }

  SpriteLayer& getLast() {
    return layers[layerCount - 1];
  }

  const SpriteLayer& operator[](size_t index) const {
    assert(index < layerCount);
    return layers[index];
  }
};

struct TilePosComponent {
  glm::i16vec2 pos;
  EntityId parent;
};

class SpriteRenderModule : public BaseRenderModule {
  static const int instanceBufferFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

  void resizeInstanceBuffer(size_t minimumNeeded) {
    capacity = std::max(capacity * 2, minimumNeeded);

    VertexBuffer oldBuffer = std::move(instanceBuffer);
    VertexBuffer newBuffer;
    newBuffer.bufferStorage(sizeof(Instance) * capacity, nullptr, instanceBufferFlags);
    oldBuffer.unmapBuffer();
    oldBuffer.bind(GL_COPY_READ_BUFFER);
    newBuffer.bind(GL_COPY_WRITE_BUFFER);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(Instance) * instanceCount);
    instanceBuffer = std::move(newBuffer);

    bindInstanceBufferAttribs();
    instances = (Instance*)instanceBuffer.mapBuffer(GL_WRITE_ONLY);
  }

  void bindInstanceBufferAttribs() {
    m_va.addVertexArrayAttrib(instanceBuffer, 2, 2, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, worldPos));
    m_va.addIntegerVertexArrayAttrib(instanceBuffer, 3, 1, GL_UNSIGNED_SHORT, sizeof(Instance), offsetof(Instance, layer));
    m_va.addVertexArrayAttrib(instanceBuffer, 4, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, z));
    m_va.addVertexArrayAttrib(instanceBuffer, 5, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, localRot));
    m_va.addVertexArrayAttrib(instanceBuffer, 6, 2, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, scale));
    m_va.attribDivisor(2, 1);
    m_va.attribDivisor(3, 1);
    m_va.attribDivisor(4, 1);
    m_va.attribDivisor(5, 1);
    m_va.attribDivisor(6, 1);
  }

public:
  struct MeshVertex {
    glm::vec2 pos;
    glm::vec2 texCoords;
  };

  struct Instance {
    glm::vec2 worldPos;
    u16 layer;
    float z;
    float localRot;
    glm::vec2 scale;
  };

  SpriteRenderModule(EntityWorld& world, size_t priority, u32 spriteWidth, u32 spriteHeight, u32 maxSprites)
    : BaseRenderModule(world, priority), m_texArray(spriteWidth, spriteHeight, maxSprites),
    m_tilemapSys(m_world.system(m_world.set<TransformComponent, TilemapComponent>(), std::bind(&SpriteRenderModule::meshTilemap, this, std::placeholders::_1, std::placeholders::_2))),
    m_spriteSys(m_world.system(m_world.set<TransformComponent, SpriteComponent>(), std::bind(&SpriteRenderModule::meshSprite, this, std::placeholders::_1, std::placeholders::_2))) {
  
    // Mesh Indicies
    std::array<u32, 6> indicies = generateIndicies();
    indexBuffer.bufferData(sizeof(indicies), indicies.data(), GL_STATIC_DRAW);

    // Mesh
    std::array<MeshVertex, 4> mesh = generateDefaultMesh();
    meshBuffer.bufferData(sizeof(mesh), mesh.data(), GL_STATIC_DRAW);
    m_va.addVertexArrayAttrib(meshBuffer, 0, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), 0);
    m_va.addVertexArrayAttrib(meshBuffer, 1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), offsetof(MeshVertex, texCoords));

    // Per Instance
    instanceBuffer.bufferStorage(sizeof(Instance) * capacity, nullptr, instanceBufferFlags);
    instances = (Instance*)instanceBuffer.mapBuffer(GL_WRITE_ONLY);
    bindInstanceBufferAttribs();

    if (!m_shader.loadShaderStr(tileVertexShaderSource, tileFragShaderSource)) {
      m_status = Status::CriticalError;
      return;
    }
    m_shader.use();
    m_shader.setInt("uSampler", 0);
    m_shader.setFloat("uTileSideLength", tileSidelength);

    m_sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    m_sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  void run(EntityWorld& world, float deltaTime) override {

  }
     
  void preMesh() override {
    
  }

  void onMesh() override {
    m_tilemapSys.run();
    m_spriteSys.run();
  }

  void meshTilemap(Entity e, float dt) {
    ShapeRenderModule& shapeRender = e.world().getModule<ShapeRenderModule>();
    TilemapComponent& tm = e.get<TilemapComponent>();
    TransformComponent& transform = e.get<TransformComponent>();
    for (glm::i16vec2 tilePos : tm.m_mainTiles) {
      Tile& tile = tm.find(tilePos);

      glm::vec2 dim = { tile.width, tile.height };
      glm::vec2 tileWorldPos = transform.getWorldPoint(tm.getLocalTileCenter(tilePos, dim));
      if (!tile.entity) {
        Instance instance;
        instance.worldPos = tileWorldPos;
        instance.layer = 0;
        instance.z = 0;
        instance.localRot = 0;
        instance.scale = { 1, 1 };
        addInstance(instance);
      } else {
        Entity tileEntity = e.world().get(tile.entity);
        SpriteComponent& sprite = tileEntity.get<SpriteComponent>();

        for (int i = 0; i < sprite.layerCount; i++) {
          Instance instance;
          instance.worldPos = tileWorldPos;
          instance.layer = sprite[i].texIndex;
          instance.z = sprite.zOffset + i * 0.1f;
          instance.localRot = sprite[i].rot;
          instance.scale = dim;
          addInstance(instance);
        }
      }

      if (tile.entity && SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F1]) {
        Entity tileE = e.world().get(tile.entity);
        if (!tileE.has<ArrowComponent>())
          continue;
        Vec2 worldPos = transform.getWorldPoint(tm.getLocalTileCenter(tilePos));
        ArrowComponent& arrow = tileE.get<ArrowComponent>();

        shapeRender.drawLine(worldPos, worldPos + arrow.dir * 0.5f, glm::vec3(1.0f, (float)arrow.generation / 255, 0.0f), 0.01f, 1.0f);
      }
    }
  }
   
  void meshSprite(Entity e, float dt) {
    ShapeRenderModule& shapeRender = e.world().getModule<ShapeRenderModule>();
    SpriteComponent& sprite = e.get<SpriteComponent>();
    TransformComponent& transform = e.get<TransformComponent>();
    if (e.has<TileBoundComponent>())
      return;

    for (int i = 0; i < sprite.layerCount; i++) {
      Instance instance;
      instance.worldPos = transform.pos;
      instance.layer = sprite[i].texIndex;
      instance.z = sprite.zOffset + i * 0.1f;
      instance.localRot = sprite[i].rot;
      instance.scale = { 1, 1 };
      addInstance(instance);
    }
  }

  void onRender(const glm::mat4& vp) override {
    instanceBuffer.unmapBuffer();

    m_shader.use();
    glActiveTexture(GL_TEXTURE0);
    m_texArray.bind();
    m_sampler.bind(0);
    m_shader.setMat4("uVP", vp);
    m_va.bind();
    indexBuffer.bind(GL_ELEMENT_ARRAY_BUFFER);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instanceCount);
    
    instanceCount = 0;
    instances = (Instance*)instanceBuffer.mapBuffer(GL_WRITE_ONLY);
  }

  u32 loadSprite(const std::string& path) {
    std::string name = getFilename(path);
    assert(!m_textureIds.contains(name));

    u32 id = m_lastFreeTexture++;
    if (!loadSprite(id, path)) {
      m_lastFreeTexture--;
      return UINT32_MAX; 
    }
    m_textureIds[name] = id;
       
    return id;
  }

  u32 getSprite(const std::string& name) {
    assert(getFilename(name) == name && "When using getSprite, use only the core filename");
    return m_textureIds.find(name)->second;
  }

  // Gets the sprite located at path, if the sprite with the core name does not exist yet
  // it is created. if the core name already exists, that sprite will be grabbed instead.
  u32 getSpriteFromPath(const std::string& path) {
    std::string name = getFilename(path);
    if (m_textureIds.contains(name))
      return getSprite(name);

    return loadSprite(path);
  }


private:
  bool loadSprite(uint32_t index, const std::string& path) {
    assert(index < 256);

    int width, height, channels;
    u8* data;

    data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
      return false;

    m_texArray.subWholeImage(index, data);

    return true;
  }

  void addInstance(Instance& instance) {
    if (instanceCount + 1 > capacity)
      resizeInstanceBuffer(instanceCount + 1);

    instances[instanceCount] = instance;
    instanceCount++;
  }

  std::array<MeshVertex, 4> generateDefaultMesh() {
    std::array<glm::vec2, 4> tileRect = {
      glm::vec2(-1, -1),
      glm::vec2(1, -1),
      glm::vec2(1,  1),
      glm::vec2(-1,  1)
    };

    std::array<glm::vec2, 4> texCoords = {
      glm::vec2(0.0f, 0.0f),
      glm::vec2(1.0f, 0.0f),
      glm::vec2(1.0f, 1.0f),
      glm::vec2(0.0f, 1.0f)
    };

    return {
      MeshVertex(tileRect[0], texCoords[0]),
      MeshVertex(tileRect[1], texCoords[1]),
      MeshVertex(tileRect[2], texCoords[2]),
      MeshVertex(tileRect[3], texCoords[3])
    };
  }

  std::array<u32, 6> generateIndicies() {
    return {
      0, 1, 2, 2, 3, 0
    };
  }

private:
  //std::array<MeshVertex, 4> mesh = generateDefaultMesh();
  //meshBuffer.bufferData(sizeof(mesh), mesh.data(), GL_STATIC_DRAW);
  //std::array<u32, 6> indicies = generateIndicies();
  //indexBuffer.bufferData(sizeof(indicies), indicies.data(), GL_STATIC_DRAW);
  //m_va.addVertexArrayAttrib(meshBuffer, 0, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), 0);
  //m_va.addVertexArrayAttrib(meshBuffer, 1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), offsetof(MeshVertex, texCoords));

  //instanceBuffer.bufferData(sizeof(Instance) * 1024, nullptr, GL_DYNAMIC_DRAW);
  //m_va.addVertexArrayAttrib(meshBuffer, 2, 2, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, pos));
  //m_va.addVertexArrayAttrib(meshBuffer, 3, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(Instance), offsetof(Instance, layer));
  //m_va.addVertexArrayAttrib(meshBuffer, 4, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, z));
  //m_va.addVertexArrayAttrib(meshBuffer, 5, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, rot));
  //glVertexAttribDivisor(2, 1);
  //glVertexAttribDivisor(3, 1);
  //glVertexAttribDivisor(4, 1);
  //glVertexAttribDivisor(5, 1);

  const char* tileVertexShaderSource = R"###(
        #version 400 core
        layout(location = 0) in vec2 pos;
        layout(location = 1) in vec2 texCoords;
        layout(location = 2) in vec2 worldPos;
        layout(location = 3) in uint layer;
        layout(location = 4) in float z;
        layout(location = 5) in float localRot;
        layout(location = 6) in vec2 scale;
        uniform mat4 uVP; 
        uniform float uTileSideLength;

        out vec3 vTexCoords;

        vec2 rotate(vec2 vec, float angle) {
          vec2 rotVec;

          float cs = cos(angle);
          float sn = sin(angle);

          rotVec.x = vec.x * cs - vec.y * sn;
          rotVec.y = vec.x * sn + vec.y * cs;

          return rotVec;
        }

        void main() {
            gl_Position = uVP * vec4(worldPos + rotate(pos * (0.5 * uTileSideLength) * scale, localRot), z, 1.0);
            vTexCoords = vec3(texCoords * scale, layer);
        })###";

  const char* tileFragShaderSource = R"###(
        #version 400 core
        in vec3 vTexCoords;

        uniform sampler2DArray uSampler;

        out vec4 fragColor;

        void main() {
            vec4 frag = texture(uSampler, vTexCoords);
            if (frag.a <= 0.01) 
              discard;

            fragColor = frag;
        })###";

private:
  u32 m_lastFreeTexture = 0;
  Map<std::string, u32> m_textureIds;

  size_t capacity = 1024;
  size_t instanceCount = 0;
  Instance* instances;
  VertexBuffer indexBuffer;
  VertexBuffer meshBuffer;
  VertexBuffer instanceBuffer;

  Shader m_shader;
  VertexArrayBuffer m_va;
  TextureArray m_texArray;
  Sampler m_sampler;
  System m_tilemapSys;
  System m_spriteSys;
};