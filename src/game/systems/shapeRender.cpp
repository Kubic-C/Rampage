#include "shapeRender.hpp"

#include "../../core/transform.hpp"
#include "../../render/module.hpp"
#include "../components/shapes.hpp"

RAMPAGE_START

struct ShapeRendererTag : SerializableTag, JsonableTag {};

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

void drawRectangle(ShapeMeshComponent& mesh, const Transform& transform, glm::vec3 color, float hw, float hh,
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

void drawLine(ShapeMeshComponent& mesh, glm::vec2 from, glm::vec2 to, glm::vec3 color, float hw,
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

void drawHollowCircle(ShapeMeshComponent& mesh, const Transform& transform, glm::vec3 color, float r,
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

void drawCircle(ShapeMeshComponent& mesh, const std::vector<ShapeVertex>& circleMesh, const Transform& transform,
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

    drawCircle(*shapeMesh, circleMesh->vertices,
               Transform(transform->getWorldPoint(circle->offset), transform->rot), circle->color,
               circle->radius, circle->z);
  }

  auto itRectangle = world->getWith(world->set<RectangleRenderComponent, TransformComponent>());
  while (itRectangle->hasNext()) {
    EntityPtr e = itRectangle->next();
    auto rect = e.get<RectangleRenderComponent>();
    Transform transform = e.get<TransformComponent>().copy();

    drawRectangle(*shapeMesh, transform, rect->color, rect->hw, rect->hh, rect->z);
  }

  return 0;
}

int renderShapes(IWorldPtr world, float dt) {
  EntityPtr shapeRender = world->getFirstWith(world->set<ShapeRendererTag>());
  auto va = shapeRender.get<VertexArrayBuffer>();
  auto shader = shapeRender.get<Shader>();
  auto mesh = shapeRender.get<ShapeMeshComponent>();

  shader->use();
  shader->setMat4("u_vp", world->getContext<RenderModule>().getViewProj());
  va->bind();
  glDrawArrays(GL_TRIANGLES, 0, mesh->verticesToRender);
  mesh->reset();

  return 0;
}

EntityPtr createShapeRender(IHost& host) {
  IWorldPtr world = host.getWorld();
  EntityPtr shapeRender = world->create();

  world->registerComponent<ShapeRendererTag>();
  world->registerComponent<VertexArrayBuffer>();
  world->registerComponent<Shader>();
  world->registerComponent<ShapeMeshComponent>();
  world->registerComponent<CircleMeshComponent>();

  shapeRender.add<ShapeRendererTag>();
  shapeRender.add<VertexArrayBuffer>();
  shapeRender.add<Shader>();
  shapeRender.add<ShapeMeshComponent>();
  shapeRender.add<CircleMeshComponent>();

  auto va = shapeRender.get<VertexArrayBuffer>();
  auto shader = shapeRender.get<Shader>();
  auto mesh = shapeRender.get<ShapeMeshComponent>();
  auto circleMesh = shapeRender.get<CircleMeshComponent>();

  va->addVertexArrayAttrib(mesh->buffer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(ShapeVertex), 0);
  va->addVertexArrayAttrib(mesh->buffer, 1, 3, GL_FLOAT, GL_FALSE, sizeof(ShapeVertex), offsetof(ShapeVertex, color));

  std::string resultStr;
  if (!shader->loadShaderStr(triangleVertexShaderSource, triangleFragShaderSource, resultStr)) {
    host.log("Shader Compilation Error:\n%s\n", resultStr.c_str());
    world->destroy(shapeRender);
    return EntityPtr(world, 0);
  }

  circleMesh->vertices = generateCircleVertices(12);

  return shapeRender;
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
