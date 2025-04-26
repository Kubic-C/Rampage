#pragma once

#include "../render/baseRender.hpp"
#include "../render/opengl.hpp"
#include "../components/transform.hpp"
#include "../components/shapes.hpp"

class ShapeRenderModule : public BaseRenderModule {
public:
  static void registerComponents(EntityWorld& world) {
    world.component<CircleRenderComponent>();
    world.component<RectangleRenderComponent>();
  }

  struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
  };

  ShapeRenderModule(EntityWorld& world, size_t priority)
    : BaseRenderModule(world, priority) {
    va.addVertexArrayAttrib(mesh.buffer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    va.addVertexArrayAttrib(mesh.buffer, 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, color));
    if (!shader.loadShaderStr(triangleVertexShaderSource, triangleFragShaderSource)) {
      m_status = Status::CriticalError;
      return;
    }

    setCircleResolution(12);
  }

  void setCircleResolution(int resolution) {
    int count = 0;
    const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
    float x = 1;
    float y = 0;
    for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
      float xNext = cos(angle + anglePerTriangle);
      float yNext = sin(angle + anglePerTriangle);

      m_circleMesh.push_back(Vertex({ 0, 0, 0 }, glm::vec3(0, 0, 0)));
      m_circleMesh.push_back(Vertex({ x, y, 0 }, glm::vec3(0, 0, 0)));
      m_circleMesh.push_back(Vertex({ xNext, yNext, 0 }, glm::vec3(0, 0, 0)));

      x = xNext;
      y = yNext;
    }
  }

  void run(EntityWorld& world, float deltaTime) override final {

  }

  void preMesh() override {
    mesh.reset();
  }

  void onMesh() override {
    EntityIterator itCircle = m_world.getWith(m_world.set<CircleRenderComponent, TransformComponent>());
    while (itCircle.hasNext()) {
      Entity e = itCircle.next();
      RefT<CircleRenderComponent> circle = e.get<CircleRenderComponent>();
      RefT<TransformComponent> transform = e.get<TransformComponent>();

      drawCircle(Transform(transform->getWorldPoint(circle->offset), transform->rot), circle->color, circle->radius, circle->z);
    }

    EntityIterator it = m_world.getWith(m_world.set<RectangleRenderComponent, TransformComponent>());
    while (it.hasNext()) {
      Entity e = it.next();
      RefT<RectangleRenderComponent> rect = e.get<RectangleRenderComponent>();
      Transform transform = e.get<TransformComponent>().copy();

      drawRectangle(transform, rect->color, rect->hw, rect->hh, rect->z);
    }
  }

  void onRender(const glm::mat4& vp) override {
    shader.use();
    shader.setMat4("u_vp", vp);
    va.bind();
    glDrawArrays(GL_TRIANGLES, 0, mesh.verticesToRender);
  }

  void drawRectangle(const Transform& transform, glm::vec3 color, float hw, float hh, float z) {
    const glm::vec3 rect[4] = {
        glm::vec3(-hw, -hh, z),
        glm::vec3(hw, -hh,  z),
        glm::vec3(hw, hh,   z),
        glm::vec3(-hw, hh,  z)
    };

    Vertex vertices[] = {
      Vertex(rect[0], color),
      Vertex(rect[1], color),
      Vertex(rect[2], color),
      Vertex(rect[2], color),
      Vertex(rect[3], color),
      Vertex(rect[0], color)
    };
    
    for (int i = 0; i < 6; i++)
      vertices[i].pos = glm::vec3(transform.getWorldPoint({ vertices[i].pos.x, vertices[i].pos.y }), vertices[i].pos.z);

    mesh.addMesh(vertices);
    mesh.addMesh(&vertices[3]);
  }

  void drawHollowCircle(const Transform& transform, glm::vec3 color, float r, int resolution = 30, float thickness = 0.01f, float z = -1) {
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

  void drawCircle(const Transform& transform, glm::vec3 color, float r, float z = -1) {
    for (int i = 0; i < m_circleMesh.size(); i += 3) {
      Vertex copy[3];
      std::memcpy(copy, &m_circleMesh[i], sizeof(Vertex) * 3);

      for (int i = 0; i < 3; i++) {
        copy[i].pos = glm::vec3(transform.getWorldPoint({ copy[i].pos.x * r, copy[i].pos.y * r}), copy[i].pos.z);
        copy[i].color = color;
      }

      mesh.addMesh(copy);
    }
  }

  void drawLine(glm::vec2 from, glm::vec2 to, glm::vec3 color, float hw, float z = -1.0f) {
    float l = glm::length(to - from);
    glm::vec2 dir = glm::normalize(to - from);
    float angle = atan2(dir.y, dir.x);

    glm::vec3 rect[4] = {
      glm::vec3(0, hw , z),
      glm::vec3(0, -hw, z),
      glm::vec3(l, -hw, z),
      glm::vec3(l, hw , z)
    };

    for (int i = 0; i < 4; i++) {
      rect[i] = glm::rotate(rect[i], angle, glm::vec3(0.0, 0.0f, 1.0f));
      rect[i] += glm::vec3(from, 0.0f);
    }

    Vertex vertices[] = {
      Vertex(rect[0], color),
      Vertex(rect[1], color),
      Vertex(rect[2], color),
      Vertex(rect[2], color),
      Vertex(rect[3], color),
      Vertex(rect[0], color)
    };

    mesh.addMesh(vertices);
    mesh.addMesh(&vertices[3]);
  }

private:
  const char* triangleVertexShaderSource = R"###(
        #version 400 core
        layout(location = 0) in vec3 a_pos;
        layout(location = 1) in vec3 a_color;

        out vec3 color;

        uniform mat4 u_vp;

        void main() {
            gl_Position = u_vp * vec4(a_pos, 1.0);
            color = a_color;
        })###";

  const char* triangleFragShaderSource = R"###(
        #version 400 core
        in vec3 color;

        out vec4 fragColor;

        void main() {
            fragColor = vec4(color, 1.0);
        })###";

private:
  std::vector<Vertex> m_circleMesh;

  Mesh<Vertex, 3, 3> mesh;
  Shader shader;
  VertexArrayBuffer va;
};
