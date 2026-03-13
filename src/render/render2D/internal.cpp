#include "internal.hpp"

RAMPAGE_START

static constexpr const char* SHAPE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/shapevs.bin";
static constexpr const char* SHAPE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/shapefs.bin";
static constexpr const char* TILE_RENDER_VERTEX_SHADER_PATH = "./res/shaders/tilevs.bin";
static constexpr const char* TILE_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/tilefs.bin";
static constexpr const char* LIGHT_RENDER_VERTEX_SHADER_PATH = "./res/shaders/lightvs.bin";
static constexpr const char* LIGHT_RENDER_FRAGMENT_SHADER_PATH = "./res/shaders/lightfs.bin";

bgfx::ShaderHandle createShaderFromFile(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) {
    throw std::runtime_error(std::string("Failed to open file: ") + path);
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  const bgfx::Memory* mem = bgfx::alloc(size);
  fread(mem->data, 1, size, file);
  fclose(file);

  return bgfx::createShader(mem);
}

void ShapeRender::generateCircleMesh(int resolution) {
  int count = 0;
  const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
  float x = 1;
  float y = 0;
  for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
    float xNext = cos(angle + anglePerTriangle);
    float yNext = sin(angle + anglePerTriangle);

    m_baseCircleMesh.push_back(ShapeVertex({0, 0, 0}, glm::vec3(0, 0, 0)));
    m_baseCircleMesh.push_back(ShapeVertex({x, y, 0}, glm::vec3(0, 0, 0)));
    m_baseCircleMesh.push_back(ShapeVertex({xNext, yNext, 0}, glm::vec3(0, 0, 0)));

    x = xNext;
    y = yNext;
  }
}

void ShapeRender::addLine(const DrawLineCmd& cmd) {
  float l = glm::length(cmd.to - cmd.from);
  glm::vec2 dir = glm::normalize(cmd.to - cmd.from);
  float angle = atan2(dir.y, dir.x);

  glm::vec3 rect[4] = {
    glm::vec3(0, cmd.thickness, cmd.z), 
    glm::vec3(0, -cmd.thickness, cmd.z), 
    glm::vec3(l, -cmd.thickness, cmd.z), 
    glm::vec3(l, cmd.thickness, cmd.z)};

  for (int i = 0; i < 4; i++) {
    rect[i] = glm::rotate(rect[i], angle, glm::vec3(0.0, 0.0f, 1.0f));
    rect[i] += glm::vec3(cmd.from, 0.0f);
  }

  std::array<ShapeVertex, 6> vertices = {
    ShapeVertex(rect[0], cmd.color), 
    ShapeVertex(rect[1], cmd.color), 
    ShapeVertex(rect[2], cmd.color),
    ShapeVertex(rect[2], cmd.color), 
    ShapeVertex(rect[3], cmd.color), 
    ShapeVertex(rect[0], cmd.color)
  };

  m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
}

ShapeRender::ShapeRender() {
  m_vertexLayout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)
      .end();
  m_program = bgfx::createProgram(createShaderFromFile(SHAPE_RENDER_VERTEX_SHADER_PATH), createShaderFromFile(SHAPE_RENDER_FRAGMENT_SHADER_PATH), true);
  generateCircleMesh(36); 
}

void ShapeRender::reset() {
  m_vertices.clear();
}

void ShapeRender::process(const std::vector<DrawRectangleCmd>& cmds) {
  for(const auto& cmd : cmds) {
    Transform transform(cmd.pos, cmd.rot);
  
    const glm::vec3 rect[4] = {
      glm::vec3(-cmd.halfSize.x, -cmd.halfSize.y, cmd.z), 
      glm::vec3( cmd.halfSize.x, -cmd.halfSize.y, cmd.z),
      glm::vec3( cmd.halfSize.x,  cmd.halfSize.y, cmd.z),          
      glm::vec3(-cmd.halfSize.x,  cmd.halfSize.y, cmd.z)
    };

    std::array<ShapeVertex, 6> vertices = {
      ShapeVertex(rect[0], cmd.color), 
      ShapeVertex(rect[1], cmd.color), 
      ShapeVertex(rect[2], cmd.color),
      ShapeVertex(rect[2], cmd.color), 
      ShapeVertex(rect[3], cmd.color), 
      ShapeVertex(rect[0], cmd.color)};

    for (int i = 0; i < 6; i++) {
      vertices[i].pos =
        glm::vec3(transform.getWorldPoint({vertices[i].pos.x, vertices[i].pos.y}), vertices[i].pos.z);
    }

    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
  }
}

void ShapeRender::process(const std::vector<DrawCircleCmd>& cmds) {
  for(const auto& cmd : cmds) {
    for (int i = 0; i < m_baseCircleMesh.size(); i += 3) {
      Transform transform(cmd.pos, 0);

      ShapeVertex copy[3];
      std::memcpy(copy, &m_baseCircleMesh[i], sizeof(ShapeVertex) * 3);

      for (int i = 0; i < 3; i++) {
        copy[i].pos = glm::vec3(transform.getWorldPoint({copy[i].pos.x * cmd.radius, copy[i].pos.y * cmd.radius}), cmd.z);
        copy[i].color = cmd.color;
      }

      m_vertices.insert(m_vertices.end(), copy, copy + 3);
    }
  }
}

void ShapeRender::process(const std::vector<DrawLineCmd>& cmds) {
  for(const auto& cmd : cmds) {
    addLine(cmd);
  }
}

void ShapeRender::process(const std::vector<DrawHollowCircleCmd>& cmds) {
  for(const auto& cmd : cmds) {
    int count = 0;
    const float anglePerTriangle = glm::pi<float>() * 2 / cmd.resolution;
    float x = cmd.radius;
    float y = 0;
    for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
      float xNext = cos(angle + anglePerTriangle) * cmd.radius;
      float yNext = sin(angle + anglePerTriangle) * cmd.radius;

      DrawLineCmd line;
      line.from = Vec2(x, y) + cmd.pos;
      line.to = Vec2(xNext, yNext) + cmd.pos;
      line.thickness = cmd.thickness;
      line.color = cmd.color;
      addLine(line);

      x = xNext;
      y = yNext;
    }
  }
}

void ShapeRender::draw() {
  if(m_vertices.empty()) return;
  bgfx::setState(DEFAULT_BLEND_STATE);
  bgfx::allocTransientVertexBuffer(&m_vertexBuffer, m_vertices.size(), m_vertexLayout);
  std::memcpy(m_vertexBuffer.data, m_vertices.data(), m_vertices.size() * sizeof(ShapeVertex));
  bgfx::setVertexBuffer(0, &m_vertexBuffer);
  bgfx::submit(0, m_program);
}

TileRender::TileRender() {

}

void TileRender::reset() {

}

void TileRender::process(const std::vector<DrawTileCmd>& cmds) {

}

void TileRender::draw() {

}

LightRender::LightRender() {

}

void LightRender::reset() {

}

void LightRender::process(const std::vector<DrawLightCmd>& cmds) {

}

void LightRender::draw() {

}

RAMPAGE_END