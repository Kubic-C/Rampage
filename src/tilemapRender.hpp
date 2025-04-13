#pragma once

#include "render/baseRender.hpp"
#include "render/opengl.hpp"
#include "transform.hpp"
#include "tilemap.hpp"

class TilemapRender : public BaseRender {
public:
  struct Vertex {
    glm::vec3 pos;
    glm::vec3 texCoords;
  };

  TilemapRender(EntityWorld& world, u32 spriteWidth, u32 spriteHeight, u32 maxSprites)
    : BaseRender(world), m_texArray(spriteWidth, spriteHeight, maxSprites) {
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

  void onMesh() override {
    m_mesh.reset();

    /* Tilemap drawing */
    EntityIterator it = m_world.getWith(m_world.set<TilemapComponent, PosComponent, RotComponent>());
    while (it.hasNext()) {
      Entity e = it.next();
      TilemapComponent& tm = e.get<TilemapComponent>();
      PosComponent& pos = e.get<PosComponent>();
      RotComponent& rot = e.get<RotComponent>();

      for (glm::i16vec2 tilePos: tm) {
        Tile& tile = tm.find(tilePos);
        if (!(tile.flags & TileFlags::IS_MAIN_TILE))
          continue;

        u16 index = 0;
        if(tile.entity)
          index = m_world.get(tile.entity).get<SpriteComponent>().texIndex;

        addTile(index, tilePos, { tile.width, tile.height }, -1);
      }
    }
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

private:
  void addTile(u16 textureId, const glm::ivec2& tilePos, const glm::ivec2& tileDim, float z = -1) {
    constexpr float hSl = tileSidelength / 2.0f;

    const std::array<glm::vec3, 4> tileRect = {
        glm::vec3(glm::vec2(0             , 0) * glm::vec2(tileDim) + (glm::vec2)tilePos * tileSidelength, z),
        glm::vec3(glm::vec2(tileSidelength, 0) * glm::vec2(tileDim) + (glm::vec2)tilePos * tileSidelength, z),
        glm::vec3(glm::vec2(tileSidelength,  tileSidelength) * glm::vec2(tileDim) + (glm::vec2)tilePos * tileSidelength, z),
        glm::vec3(glm::vec2(0             ,  tileSidelength) * glm::vec2(tileDim) + (glm::vec2)tilePos * tileSidelength, z)
    };

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
  Mesh<Vertex, 6, 6> m_mesh;
  Shader m_shader;
  VertexArrayBuffer m_va;
  TextureArray m_texArray;
  Sampler m_sampler;
};