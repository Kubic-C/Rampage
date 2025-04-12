#pragma once

#include "../utility/base.hpp"
#include "../utility/log.hpp"
#include "../transform.hpp"
#include "../tile.hpp"
#include "camera.hpp"
#include "stb_image.h"

uint32_t createShaderFromSource(uint32_t shader_type, const char* src, int* size = nullptr) {
  uint32_t shaderid;

  shaderid = glCreateShader(shader_type);
  glShaderSource(shaderid, 1, &src, size);
  glCompileShader(shaderid);

  {
    int success;
    char info_log[512];
    glGetShaderiv(shaderid, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shaderid, 512, NULL, info_log);
      printf("shader compilation failed: %s\n", info_log);
      glDeleteShader(shaderid);
      return -1;
    }
  }

  return shaderid;
}

uint32_t createShader(uint32_t shader_type, const char* file_path) {
  int file_size;
  char* file_data;

  {
    FILE* file;

    file = fopen(file_path, "r");
    if (!file) {
      return -1;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    file_data = (char*)malloc(file_size * sizeof(char));
    fread(file_data, sizeof(char), file_size, file);
    fclose(file);
  }

  return createShaderFromSource(shader_type, file_data, &file_size);
}

uint32_t createProgram(const std::vector<uint32_t>& shaders, bool deleteShaders = true) {
  uint32_t shaderProgram = glCreateProgram();

  for (uint32_t shader : shaders) {
    glAttachShader(shaderProgram, shader);
  }

  glLinkProgram(shaderProgram);

  int success;
  char info_log[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, info_log);
    printf("program failure to compile %s", info_log);
    return UINT32_MAX;
  }

  return shaderProgram;
}

void GLAPIENTRY glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar* message, const void* userParam) {
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return;

  logGeneric("<bgWhite, fgBlue>OpenGL report:<reset>\n");
  logError(1, "\tsoure: 0x%x\n", source);
  logError(1, "\ttype: 0x%x\n", type);
  logError(1, "\tid: 0x%x\n", id);
  logError(1, "\tseverity: 0x%x\n", severity);
  logError(1, "\t%s\n", message);
}

class Render {
public:
  static constexpr int MinimumMajorGLVersion = 4;
  static constexpr int MinimumMinorGLVersion = 0;

  static SDL_WindowFlags getNecessaryWindowFlags() {
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    return flags;
  }

private:
  const char* triangleVertexShaderSource = R"###(
        #version 400 core
        layout(location = 0) in vec3 a_pos;
        layout(location = 1) in vec3 a_color;

        out vec3 color;

        uniform mat4 u_vp;
        uniform mat4 u_model;

        void main() {
            gl_Position = u_vp * u_model * vec4(a_pos, 1.0);
            color = a_color;
        })###";

  const char* triangleFragShaderSource = R"###(
        #version 400 core
        in vec3 color;

        out vec4 fragColor;

        void main() {
            fragColor = vec4(color, 1.0);
        })###";

    const char* tileVertexShaderSource = R"###(
        #version 400 core
        layout(location = 0) in vec3 a_pos;
        layout(location = 1) in vec3 a_tex_coords;

        out vec3 v_tex_coords;

        uniform mat4 u_vp;
        uniform mat4 u_model;

        void main() {
            gl_Position = u_vp * u_model * vec4(a_pos, 1.0);
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

  template<typename T>
  void setVertexAt(T* data, u32 index, const T& vertex) {
    data[index] = vertex;
  }

public:

  Render(SDL_Window* window, Entity camera)
    : m_window(window), m_camera(camera), m_status(Status::Ok) {
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
      m_status = Status::CriticalError;
      logError(1, "Failed to load OpenGL Context: %i %i", MinimumMajorGLVersion, MinimumMinorGLVersion);
      return;
    }

    resizeViewportToScreenDim();

    /* Basic state */
#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glErrorCallback, nullptr);
#endif 

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Simple triangle rendering */
    glCreateVertexArrays(1, &m_triangleBuffer.vao);
    glBindVertexArray(m_triangleBuffer.vao);
  
    glGenBuffers(1, &m_triangleBuffer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_triangleBuffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_triangleBuffer.buffer), m_triangleBuffer.buffer, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle::Vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Triangle::Vertex), (const void*)offsetof(Triangle::Vertex, color));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    m_triangleBuffer.shader = createProgram(
      { createShaderFromSource(GL_VERTEX_SHADER, triangleVertexShaderSource),
       createShaderFromSource(GL_FRAGMENT_SHADER, triangleFragShaderSource) });
    if (m_triangleBuffer.shader == UINT32_MAX) {
      m_status = Status::CriticalError;
      return;
    }

    m_triangleBuffer.u_vp = glGetUniformLocation(m_triangleBuffer.shader, "u_vp");
    m_triangleBuffer.u_model = glGetUniformLocation(m_triangleBuffer.shader, "u_model");

    /* Tile rendering */
    glCreateVertexArrays(1, &m_tileBuffer.vao);
    glBindVertexArray(m_tileBuffer.vao);

    glGenBuffers(1, &m_tileBuffer.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_tileBuffer.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Tile::Vertex) * m_tileBuffer.capacity, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Tile::Vertex), (const void*)offsetof(Tile::Vertex, texCoords));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    m_tileBuffer.shader = createProgram(
      { createShaderFromSource(GL_VERTEX_SHADER, tileVertexShaderSource),
       createShaderFromSource(GL_FRAGMENT_SHADER, tileFragShaderSource) });
    if (m_tileBuffer.shader == UINT32_MAX) {
      m_status = Status::CriticalError;
      return;
    }

    m_tileBuffer.u_vp = glGetUniformLocation(m_tileBuffer.shader, "u_vp");
    m_tileBuffer.u_model = glGetUniformLocation(m_tileBuffer.shader, "u_model");
    m_tileBuffer.u_sampler = glGetUniformLocation(m_tileBuffer.shader, "u_sampler");

    createSpriteArray(32, 32, 256);

    glUseProgram(m_tileBuffer.shader);
    glUniform1i(m_tileBuffer.u_sampler, 0);
  }

  Status getStatus() {
    return m_status;
  }

  void clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void display() {
    SDL_GL_SwapWindow(m_window);
  }

  void setCamera(Entity camera) {
    m_camera = camera;
  }

  void drawRectangle(const Transform& transform, glm::vec3 color, float hw, float hh) {
    if (!m_camera)
      return;

    const glm::vec3 rect[4] = {
        glm::vec3(-hw, -hh, -1.0f),
        glm::vec3(hw, -hh, -1.0f),
        glm::vec3(hw, hh, -1.0f),
        glm::vec3(-hw, hh, -1.0f)
    };

    setVertexAt(m_triangleBuffer.buffer, 0, Triangle::Vertex(rect[0], color));
    setVertexAt(m_triangleBuffer.buffer, 1, Triangle::Vertex(rect[1], color));
    setVertexAt(m_triangleBuffer.buffer, 2, Triangle::Vertex(rect[2], color));
    setVertexAt(m_triangleBuffer.buffer, 3, Triangle::Vertex(rect[2], color));
    setVertexAt(m_triangleBuffer.buffer, 4, Triangle::Vertex(rect[3], color));
    setVertexAt(m_triangleBuffer.buffer, 5, Triangle::Vertex(rect[0], color));
    glBindBuffer(GL_ARRAY_BUFFER, m_triangleBuffer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Triangle::Vertex) * 6, m_triangleBuffer.buffer);

    glm::mat4 model = glm::identity<glm::mat4>();
    model = glm::translate(model, glm::vec3(transform.pos, 0.0f));
    model = glm::rotate(model, transform.rot.radians(), glm::vec3(0.0f, 0.0f, 1.0f));

    float cameraZoom = m_camera.get<CameraComponent>().m_zoom;
    glm::mat4 vp = m_proj * getView();

    glUseProgram(m_triangleBuffer.shader);
    glUniformMatrix4fv(m_triangleBuffer.u_vp, 1, GL_FALSE, glm::value_ptr(vp));
    glUniformMatrix4fv(m_triangleBuffer.u_model, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(m_triangleBuffer.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  void drawHollowCircle(const Transform& transform, glm::vec3 color, float r, int resolution = 30, float thickness = 0.01f, float z = -1) {
    if (!m_camera)
      return;

    int count = 0;
    const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
    float x = r;
    float y = 0;
    for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
      float xNext = cos(angle + anglePerTriangle) * r;
      float yNext = sin(angle + anglePerTriangle) * r;

      drawLine(Vec2(x, y) + transform.pos, Vec2(xNext, yNext) + transform.pos, color, thickness);

      x = xNext;
      y = yNext;
    }
  }

  void drawCircle(const Transform& transform, glm::vec3 color, float r, int resolution = 10, float z = -1) {
    if (!m_camera)
      return;
    
    int count = 0;
    const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
    float x = r;
    float y = 0;
    for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
      float xNext = cos(angle + anglePerTriangle) * r;
      float yNext = sin(angle + anglePerTriangle) * r;

      setVertexAt(m_triangleBuffer.buffer, count, Triangle::Vertex({ 0, 0, z }, color));
      setVertexAt(m_triangleBuffer.buffer, count + 1, Triangle::Vertex({ x, y, z}, color));
      setVertexAt(m_triangleBuffer.buffer, count + 2, Triangle::Vertex({ xNext, yNext, z }, color));

      x = xNext;
      y = yNext;
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_triangleBuffer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Triangle::Vertex) * count, m_triangleBuffer.buffer);

    glm::mat4 model = glm::identity<glm::mat4>();
    model = glm::translate(model, glm::vec3(transform.pos, 0.0f));
    model = glm::rotate(model, transform.rot.radians(), glm::vec3(0.0f, 0.0f, 1.0f));

    float cameraZoom = m_camera.get<CameraComponent>().m_zoom;
    glm::mat4 vp = m_proj * getView();

    glUseProgram(m_triangleBuffer.shader);
    glUniformMatrix4fv(m_triangleBuffer.u_vp, 1, GL_FALSE, glm::value_ptr(vp));
    glUniformMatrix4fv(m_triangleBuffer.u_model, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(m_triangleBuffer.vao);
    glDrawArrays(GL_TRIANGLES, 0, count * 3);
  }

  void drawLine(glm::vec2 from, glm::vec2 to, glm::vec3 color, float hw) {
    if (!m_camera)
      return;

    float l = glm::length(to - from);
    glm::vec2 dir = glm::normalize(to - from);
    float angle = atan2(dir.y, dir.x);

    glm::vec3 rect[4] = {
      glm::vec3(0, hw, -1.0f),
      glm::vec3(0, -hw, -1.0f),
      glm::vec3(l, -hw, -1.0f),
      glm::vec3(l, hw, -1.0f)
    };

    for (int i = 0; i < 4; i++) {
      rect[i] = glm::rotate(rect[i], angle, glm::vec3(0.0, 0.0f, 1.0f));
      rect[i] += glm::vec3(from, 0.0f);
    }

    setVertexAt(m_triangleBuffer.buffer, 0, Triangle::Vertex(rect[0], color));
    setVertexAt(m_triangleBuffer.buffer, 1, Triangle::Vertex(rect[1], color));
    setVertexAt(m_triangleBuffer.buffer, 2, Triangle::Vertex(rect[2], color));
    setVertexAt(m_triangleBuffer.buffer, 3, Triangle::Vertex(rect[2], color));
    setVertexAt(m_triangleBuffer.buffer, 4, Triangle::Vertex(rect[3], color));
    setVertexAt(m_triangleBuffer.buffer, 5, Triangle::Vertex(rect[0], color));
    glBindBuffer(GL_ARRAY_BUFFER, m_triangleBuffer.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Triangle::Vertex) * 6, m_triangleBuffer.buffer);

    glm::mat4 model = glm::identity<glm::mat4>();

    float cameraZoom = m_camera.get<CameraComponent>().m_zoom;
    glm::mat4 vp = m_proj * getView();

    glUseProgram(m_triangleBuffer.shader);
    glUniformMatrix4fv(m_triangleBuffer.u_vp, 1, GL_FALSE, glm::value_ptr(vp));
    glUniformMatrix4fv(m_triangleBuffer.u_model, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(m_triangleBuffer.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  void startTileBatch() {
    m_tileBuffer.buffer.clear();
  }

  void addTile(u16 textureId, const glm::ivec2& tilePos, const glm::ivec2& tileDim, float z = -1) {
    if (!m_camera)
      return;

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

    u32 startIndex = m_tileBuffer.buffer.size();
    m_tileBuffer.buffer.resize(m_tileBuffer.buffer.size() + 6);
    setVertexAt(m_tileBuffer.buffer.data(), startIndex    , Tile::Vertex(tileRect[0], texCoords[0]));
    setVertexAt(m_tileBuffer.buffer.data(), startIndex + 1, Tile::Vertex(tileRect[1], texCoords[1]));
    setVertexAt(m_tileBuffer.buffer.data(), startIndex + 2, Tile::Vertex(tileRect[2], texCoords[2]));
    setVertexAt(m_tileBuffer.buffer.data(), startIndex + 3, Tile::Vertex(tileRect[2], texCoords[2]));
    setVertexAt(m_tileBuffer.buffer.data(), startIndex + 4, Tile::Vertex(tileRect[3], texCoords[3]));
    setVertexAt(m_tileBuffer.buffer.data(), startIndex + 5, Tile::Vertex(tileRect[0], texCoords[0]));
  }

  void drawTileBatch(const Transform& transform) {
    if (!m_camera)
      return;

    glBindBuffer(GL_ARRAY_BUFFER, m_tileBuffer.vbo);
    size_t updateSize = m_tileBuffer.buffer.size() * sizeof(Tile::Vertex);
    if (updateSize > m_tileBuffer.capacity) {
      m_tileBuffer.capacity = updateSize;
      glBufferData(GL_ARRAY_BUFFER, m_tileBuffer.buffer.size() * sizeof(Tile::Vertex), m_tileBuffer.buffer.data(), GL_DYNAMIC_DRAW);
    } else {
      glBufferSubData(GL_ARRAY_BUFFER, 0, m_tileBuffer.buffer.size() * sizeof(Tile::Vertex), m_tileBuffer.buffer.data());
    }


    glm::mat4 model = glm::identity<glm::mat4>();
    model = glm::translate(model, glm::vec3(transform.pos, 0.0f));
    model = glm::rotate(model, transform.rot.radians(), glm::vec3(0.0f, 0.0f, 1.0f));

    float cameraZoom = m_camera.get<CameraComponent>().m_zoom;
    glm::mat4 vp = m_proj * getView();

    glUseProgram(m_tileBuffer.shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileBuffer.textureArray);
    glBindSampler(0, m_tileBuffer.textureSampler);
    glUniformMatrix4fv(m_tileBuffer.u_vp, 1, GL_FALSE, glm::value_ptr(vp));
    glUniformMatrix4fv(m_tileBuffer.u_model, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(m_tileBuffer.vao);
    glDrawArrays(GL_TRIANGLES, 0, m_tileBuffer.buffer.size());
  }

  bool loadSprite(uint32_t index, const std::string& path) {
    assert(index < 256);

    int width, height, channels;
    u8* data;

    data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
      return false;

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileBuffer.textureArray);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, (const void*)data);

    return true;
  }

  glm::mat4 getView() {
    glm::ivec2 screenDim;
    SDL_GetWindowSize(m_window, &screenDim.x, &screenDim.y);
    CameraComponent& camera = m_camera.get<CameraComponent>();
    PosComponent pos = m_camera.get<PosComponent>();
    RotComponent rot = m_camera.get<RotComponent>();

    return camera.view(Transform(pos, camera.m_rot), (glm::vec2)screenDim);
  }

  void resizeViewport(const glm::ivec2& size) {
    glm::vec2 fsize = (glm::vec2)size * 0.5f;
    m_proj = glm::ortho<float>(-fsize.x, fsize.x, -fsize.y, fsize.y, 0.0001f, 10000.0f);
    glViewport(0, 0, size.x, size.y);
  }

  void resizeViewportToScreenDim() {
    glm::ivec2 screenSize;
    SDL_GetWindowSize(m_window, &screenSize.x, &screenSize.y);

    resizeViewport(screenSize);
  }

  glm::vec2 getWorldCoords(const glm::ivec2& _screenCoords) {
    glm::ivec2 screenSize;
    SDL_GetWindowSize(m_window, &screenSize.x, &screenSize.y);

    glm::vec2 screenCoords = { (float)_screenCoords.x, (float)_screenCoords.y };

    glm::mat4 view = getView();

    screenCoords.y = screenSize.y - screenCoords.y;

    glm::vec2 worldCoords =
      glm::unProject(
        glm::vec3(screenCoords, -1.0f),
        view,
        m_proj,
        glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

    return worldCoords;
  }

  glm::vec2 getScreenCoords(const glm::ivec2& worldCoords) {
    glm::ivec2 screenSize;
    SDL_GetWindowSize(m_window, &screenSize.x, &screenSize.y);

    glm::mat4 view = getView();

    glm::vec2 screenCoords =
      glm::project(
        glm::vec3(screenCoords, -1.0f),
        view,
        m_proj,
        glm::vec4(0.0f, 0.0f, (float)screenSize.x, (float)screenSize.y));

    screenCoords.y = screenSize.y - screenCoords.y;

    return screenCoords;
  }

private:
  void createSpriteArray(uint32_t maxSpriteWidth, uint32_t maxSpriteHeight, uint32_t maxSpriteCount) {
    glGenTextures(1, &m_tileBuffer.textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileBuffer.textureArray);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 4, GL_RGBA8, maxSpriteWidth, maxSpriteHeight, maxSpriteCount);

    glCreateSamplers(1, &m_tileBuffer.textureSampler);
    glSamplerParameteri(m_tileBuffer.textureSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(m_tileBuffer.textureSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(m_tileBuffer.textureSampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(m_tileBuffer.textureSampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }

private:
  Status m_status;

  glm::mat4 m_proj;
  Entity m_camera;

  struct Triangle {
    struct Vertex {
      glm::vec3 pos;
      glm::vec3 color;
    };

    Vertex buffer[1024];

    u32 vao;
    u32 vbo;
    u32 shader;

    u32 u_vp;
    u32 u_model;
  } m_triangleBuffer;

  struct Tile {
    struct Vertex {
      glm::vec3 pos;
      glm::vec3 texCoords;
    };

    int capacity = 1024 * 64;
    std::vector<Vertex> buffer;

    u32 vao;
    u32 vbo;
    u32 shader;
    u32 textureArray;
    u32 textureSampler;

    u32 u_vp;
    u32 u_model;
    u32 u_sampler;
  } m_tileBuffer;

  SDL_Window* m_window;
};