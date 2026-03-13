#include "shapeRender.hpp"

#include "../../core/transform.hpp"
#include "../../render/module.hpp"
#include "../components/shapes.hpp"

RAMPAGE_START

int renderShapes(IWorldPtr world, float dt) {
  auto& render2D = world->getContext<Render2D>();
  
  auto it = world->getWith<TransformComponent, RectangleRenderComponent>();
  while (it->hasNext()) {
    EntityPtr e = it->next();
    auto rect = e.get<RectangleRenderComponent>();
    Transform transform = e.get<TransformComponent>().copy();

    DrawRectangleCmd cmd;
    cmd.pos = transform.pos;
    cmd.rot = transform.rot;
    cmd.halfSize = glm::vec2(rect->hw, rect->hh) * 2.0f;
    cmd.color = glm::vec4(rect->color, 1.0f);
    cmd.z = rect->z;
    render2D.submit(cmd);
  }

  it = world->getWith<TransformComponent, CircleRenderComponent>();
   while (it->hasNext()) {
    EntityPtr e = it->next();
    auto circle = e.get<CircleRenderComponent>();
    Transform transform = e.get<TransformComponent>().copy();

    DrawCircleCmd cmd;
    cmd.pos = transform.pos;
    cmd.radius = circle->radius;
    cmd.color = glm::vec4(circle->color, 1.0f);
    cmd.z = circle->z;
    render2D.submit(cmd);
  }

  return 0;
}

bool loadShapeRender(IHost& host) {
  Pipeline& pipeline = host.getPipeline();

  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnRenderStage>(renderShapes);

  return true;
}

RAMPAGE_END
