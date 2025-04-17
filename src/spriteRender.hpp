#pragma once

#include "render/shapes.hpp"
#include "transform.hpp"
#include "tilemap.hpp"
#include "seekPlayer.hpp"

class SpriteRenderModule : public BaseRenderModule {
public:
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 texCoords;
  };

  SpriteRenderModule(EntityWorld& world, size_t priority, u32 spriteWidth, u32 spriteHeight, u32 maxSprites)
    : BaseRenderModule(world, priority), m_texArray(spriteWidth, spriteHeight, maxSprites),
    m_tilemapSys(m_world.system(m_world.set<TransformComponent, TilemapComponent>(), std::bind(&SpriteRenderModule::meshTilemap, this, std::placeholders::_1, std::placeholders::_2))),
    m_spriteSys(m_world.system(m_world.set<TransformComponent, TileSpriteComponent>(), std::bind(&SpriteRenderModule::meshSprite, this, std::placeholders::_1, std::placeholders::_2))),
    m_layeredSpriteSys(m_world.system(m_world.set<TransformComponent, LayeredSpriteComponent>(), std::bind(&SpriteRenderModule::meshSpriteLayered, this, std::placeholders::_1, std::placeholders::_2))) {
    m_va.addVertexArrayAttrib(m_mesh.buffer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    m_va.addVertexArrayAttrib(m_mesh.buffer, 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoords));
    if (!m_shader.loadShaderStr(tileVertexShaderSource, tileFragShaderSource)) {
      m_status = Status::CriticalError;
      return;
    }
    m_shader.use();
    m_shader.setInt("u_sampler", 0);

    m_sampler.parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_sampler.parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_sampler.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    m_sampler.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

  void run(EntityWorld& world, float deltaTime) override {

  }
     
  void preMesh() override {
    m_mesh.reset();
  }

  void onMesh() override {
    m_tilemapSys.run();
    m_spriteSys.run();
    m_layeredSpriteSys.run();
  }

  void meshTilemap(Entity e, float dt) {
    ShapeRenderModule& shapeRender = e.world().getModule<ShapeRenderModule>();
    TilemapComponent& tm = e.get<TilemapComponent>();
    TransformComponent& transform = e.get<TransformComponent>();
    for (glm::i16vec2 tilePos : tm) {
      Tile& tile = tm.find(tilePos);
      if (!(tile.flags & TileFlags::IS_MAIN_TILE))
        continue;

      if (!tile.entity) {
        addTile(0, tilePos, { tile.width, tile.height }, Vec2(0), 0, -1);
      } else {
        Entity tileEntity = e.world().get(tile.entity);
        assert(tileEntity.has<TileSpriteComponent>() || tileEntity.has<LayeredSpriteComponent>());
        
        if (tileEntity.has<TileSpriteComponent>()) {
          TileSpriteComponent& sprite = tileEntity.get<TileSpriteComponent>();
          addTile(sprite.texIndex, tilePos, { tile.width, tile.height }, sprite.offset, sprite.rot, -1);
        }
        else {
          LayeredSpriteComponent& ls = tileEntity.get<LayeredSpriteComponent>();
          for (int i = 0; i < ls.count; i++)
            addTile(ls.layers[i].texIndex, tilePos, { tile.width, tile.height }, ls.layers[i].offset, ls.layers[i].rot, -1 + i * 0.1f);
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
    TileSpriteComponent& ts = e.get<TileSpriteComponent>();
    TransformComponent& transform = e.get<TransformComponent>();

    addSprite(ts.texIndex, transform.pos, ts.offset, ts.rot);
  }

  void meshSpriteLayered(Entity e, float dt) {
    ShapeRenderModule& shapeRender = e.world().getModule<ShapeRenderModule>();
    LayeredSpriteComponent& ls = e.get<LayeredSpriteComponent>();
    TransformComponent& transform = e.get<TransformComponent>();

    for(int i = 0; i < ls.count; i++) 
      addSprite(ls.layers[i].texIndex, transform.pos, ls.layers[i].offset, ls.layers[i].rot);
  }

  void onRender(const glm::mat4& vp) override {
    m_shader.use();
    glActiveTexture(GL_TEXTURE0);
    m_texArray.bind();
    m_sampler.bind(0);
    m_shader.setMat4("u_vp", vp);
    m_va.bind();
    glDrawArrays(GL_TRIANGLES, 0, m_mesh.verticesToRender);
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

  void addTile(u16 textureId, const glm::ivec2& tilePos, const glm::ivec2& tileDim, const glm::vec2& offset, float rot, float z = -1) {
    constexpr float hSl = tileSidelength / 2.0f;

    std::array<glm::vec3, 4> tileRect = {
        glm::vec3(glm::vec2(0             , 0), z),
        glm::vec3(glm::vec2(tileSidelength, 0), z),
        glm::vec3(glm::vec2(tileSidelength,  tileSidelength), z),
        glm::vec3(glm::vec2(0             ,  tileSidelength), z)
    };

    for (glm::vec3& pos : tileRect) {
      glm::vec2 pos2d = { pos.x, pos.y };
      pos2d = fast2DRotate(pos2d, rot);
      pos2d *= tileDim;
      pos2d += (glm::vec2)tilePos * tileSidelength + offset;
      pos = glm::vec3(pos2d, z);
    }

    std::array<glm::vec3, 4> texCoords = {
      glm::vec3(0.0f, 0.0f, (float)textureId) * glm::vec3(tileDim, 1.0f),
      glm::vec3(1.0f, 0.0f, (float)textureId) * glm::vec3(tileDim, 1.0f),
      glm::vec3(1.0f, 1.0f, (float)textureId) * glm::vec3(tileDim, 1.0f),
      glm::vec3(0.0f, 1.0f, (float)textureId) * glm::vec3(tileDim, 1.0f)
    };

    Vertex vertices[] = {
      Vertex(tileRect[0], texCoords[0]),
      Vertex(tileRect[1], texCoords[1]),
      Vertex(tileRect[2], texCoords[2]),
      Vertex(tileRect[2], texCoords[2]),
      Vertex(tileRect[3], texCoords[3]),
      Vertex(tileRect[0], texCoords[0])
    };

    m_mesh.addMesh(vertices);
  }

  void addSprite(u16 textureId, const glm::vec2& pos, const glm::vec2& offset, float rot, float z = -1) {
    constexpr float hSl = tileSidelength / 2.0f;

    std::array<glm::vec3, 4> tileRect = {
        glm::vec3(glm::vec2(0             , 0) + (glm::vec2)pos, z),
        glm::vec3(glm::vec2(tileSidelength, 0) + (glm::vec2)pos, z),
        glm::vec3(glm::vec2(tileSidelength,  tileSidelength) + (glm::vec2)pos, z),
        glm::vec3(glm::vec2(0             ,  tileSidelength) + (glm::vec2)pos, z)
    };

    for (glm::vec3& pos : tileRect) {
      glm::vec2 pos2d = { pos.x, pos.y };
      pos2d = fast2DRotate(pos2d, rot);
      pos2d += pos2d + offset;
      pos = glm::vec3(pos2d, z);
    }

    std::array<glm::vec3, 4> texCoords = {
      glm::vec3(0.0f, 0.0f, (float)textureId),
      glm::vec3(1.0f, 0.0f, (float)textureId),
      glm::vec3(1.0f, 1.0f, (float)textureId),
      glm::vec3(0.0f, 1.0f, (float)textureId)
    };

    Vertex vertices[] = {
      Vertex(tileRect[0], texCoords[0]),
      Vertex(tileRect[1], texCoords[1]),
      Vertex(tileRect[2], texCoords[2]),
      Vertex(tileRect[2], texCoords[2]),
      Vertex(tileRect[3], texCoords[3]),
      Vertex(tileRect[0], texCoords[0])
    };

    m_mesh.addMesh(vertices);
  }

private:
  const char* tileVertexShaderSource = R"###(
        #version 400 core
        layout(location = 0) in vec3 a_pos;
        layout(location = 1) in vec3 a_tex_coords;

        out vec3 v_tex_coords;

        uniform mat4 u_vp;

        void main() {
            gl_Position = u_vp * vec4(a_pos, 1.0);
            v_tex_coords = a_tex_coords;
        })###";

  const char* tileFragShaderSource = R"###(
        #version 400 core
        in vec3 v_tex_coords;

        uniform sampler2DArray u_sampler;

        out vec4 fragColor;

        void main() {
            fragColor = texture(u_sampler, v_tex_coords);
        })###";

private:
  u32 m_lastFreeTexture = 0;
  Map<std::string, u32> m_textureIds;
  Mesh<Vertex, 6, 6> m_mesh;
  Shader m_shader;
  VertexArrayBuffer m_va;
  TextureArray m_texArray;
  Sampler m_sampler;
  System m_tilemapSys;
  System m_spriteSys;
  System m_layeredSpriteSys;
};