#include "shapeRender.hpp"

#include "../../core/transform.hpp"
#include "../../render/module.hpp"
#include "../components/shapes.hpp"

RAMPAGE_START

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

        
std::vector<ShapeVertex> generateCircleVertices(int resolution) {
  std::vector<ShapeVertex> circleMesh;

  int count = 0;
  const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
  float x = 1;
  float y = 0;
  for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
    float xNext = cos(angle + anglePerTriangle);
    float yNext = sin(angle + anglePerTriangle);

    circleMesh.push_back(ShapeVertex({0, 0, 0}, glm::vec3(0, 0, 0)));
    circleMesh.push_back(ShapeVertex({x, y, 0}, glm::vec3(0, 0, 0)));
    circleMesh.push_back(ShapeVertex({xNext, yNext, 0}, glm::vec3(0, 0, 0)));

    x = xNext;
    y = yNext;
  }

  return circleMesh;
}

struct ShapeRenderContext {
  bgfx::VertexLayout layout;
  bgfx::DynamicVertexBufferHandle buffer;
  bgfx::ProgramHandle shader;

  size_t vertexCount = 0;
  std::vector<ShapeVertex> mesh;
};

void drawRectangle(IWorldPtr world, const Transform& transform, glm::vec3 color, float hw, float hh,
                   float z) {
  const glm::vec3 rect[4] = {glm::vec3(-hw, -hh, z), glm::vec3(hw, -hh, z), glm::vec3(hw, hh, z),
                             glm::vec3(-hw, hh, z)};

  ShapeVertex vertices[] = {ShapeVertex(rect[0], color), ShapeVertex(rect[1], color), ShapeVertex(rect[2], color),
                            ShapeVertex(rect[2], color), ShapeVertex(rect[3], color), ShapeVertex(rect[0], color)};

  for (int i = 0; i < 6; i++)
    vertices[i].pos =
        glm::vec3(transform.getWorldPoint({vertices[i].pos.x, vertices[i].pos.y}), vertices[i].pos.z);

  mesh.addMesh(vertices);
  mesh.addMesh(&vertices[3]);
}

void drawLine(IWorldPtr world, glm::vec2 from, glm::vec2 to, glm::vec3 color, float hw,
              float z) {
  float l = glm::length(to - from);
  glm::vec2 dir = glm::normalize(to - from);
  float angle = atan2(dir.y, dir.x);

  glm::vec3 rect[4] = {glm::vec3(0, hw, z), glm::vec3(0, -hw, z), glm::vec3(l, -hw, z), glm::vec3(l, hw, z)};

  for (int i = 0; i < 4; i++) {
    rect[i] = glm::rotate(rect[i], angle, glm::vec3(0.0, 0.0f, 1.0f));
    rect[i] += glm::vec3(from, 0.0f);
  }

  ShapeVertex vertices[] = {ShapeVertex(rect[0], color), ShapeVertex(rect[1], color), ShapeVertex(rect[2], color),
                            ShapeVertex(rect[2], color), ShapeVertex(rect[3], color), ShapeVertex(rect[0], color)};

  mesh.addMesh(vertices);
  mesh.addMesh(&vertices[3]);
}
  
void drawHollowCircle(IWorldPtr world, const Transform& transform, glm::vec3 color, float r,
                      int resolution, float thickness, float z) {
  int count = 0;
  const float anglePerTriangle = glm::pi<float>() * 2 / resolution;
  float x = r;
  float y = 0;
  for (float angle = 0.0f; angle <= 2 * glm::pi<float>(); angle += anglePerTriangle, count += 3) {
    float xNext = cos(angle + anglePerTriangle) * r;
    float yNext = sin(angle + anglePerTriangle) * r;

    drawLine(mesh, Vec2(x, y) + transform.pos, Vec2(xNext, yNext) + transform.pos, color, thickness);

    x = xNext;
    y = yNext;
  }
}

void drawCircle(IWorldPtr world, const std::vector<ShapeVertex>& circleMesh, const Transform& transform,
                glm::vec3 color, float r, float z) {
  for (int i = 0; i < circleMesh.size(); i += 3) {
    ShapeVertex copy[3];
    std::memcpy(copy, &circleMesh[i], sizeof(ShapeVertex) * 3);

    for (int i = 0; i < 3; i++) {
      copy[i].pos = glm::vec3(transform.getWorldPoint({copy[i].pos.x * r, copy[i].pos.y * r}), z);
      copy[i].color = color;
    }

    mesh.addMesh(copy);
  }
}

int meshShapes(IWorldPtr world, float dt) {
  EntityPtr shapeRender = world->getFirstWith(world->set<ShapeRendererTag>());
  auto shapeMesh = shapeRender.get<ShapeMeshComponent>();
  auto circleMesh = shapeRender.get<CircleMeshComponent>();

  auto itCircle = world->getWith(world->set<CircleRenderComponent, TransformComponent>());
  while (itCircle->hasNext()) {
    EntityPtr e = itCircle->next();
    auto circle = e.get<CircleRenderComponent>();
    auto transform = e.get<TransformComponent>();

    drawCircle(world, circleMesh->vertices,
               Transform(transform->getWorldPoint(circle->offset), transform->rot), circle->color,
               circle->radius, circle->z);
  }

  auto itRectangle = world->getWith(world->set<RectangleRenderComponent, TransformComponent>());
  while (itRectangle->hasNext()) {
    EntityPtr e = itRectangle->next();
    auto rect = e.get<RectangleRenderComponent>();
    Transform transform = e.get<TransformComponent>().copy();

    drawRectangle(world, transform, rect->color, rect->hw, rect->hh, rect->z);
  }

  return 0;
}

int renderShapes(IWorldPtr world, float dt) {
  EntityPtr shapeRender = world->getFirstWith(world->set<ShapeRendererTag>());

  return 0;
}

void createShapeRender(IHost& host) {
  IWorldPtr world = host.getWorld();
  world->addContext<ShapeRenderContext>();
  auto& shapeRender = world->getContext<ShapeRenderContext>();

  shapeRender.layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)
      .end();

  shapeRender.  
}

bool loadShapeRender(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  EntityPtr shapeRender = createShapeRender(host);
  if (shapeRender.isNull())
    return false;


  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::PreRenderStage>(meshShapes);
  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnRenderStage>(renderShapes);

  return true;
}

RAMPAGE_END
