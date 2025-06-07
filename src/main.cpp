#include "app.hpp"

#include "ecs/serter.hpp"

int main() {
  /*{
    EntityWorldSerializable m_world;

    m_world.component<TransformComponent>();
    m_world.registerSerializable<TransformComponent>(
      [](capnp::MessageBuilder& builder, Ref component) {
        auto transformBuilder = builder.initRoot<Schema::Transform>();
        auto transform = component.cast<TransformComponent>();

        transformBuilder.getPos().setX(transform->pos.x);
        transformBuilder.getPos().setY(transform->pos.y);
        transformBuilder.setRot(transform->rot);
      },
      [](capnp::MessageReader& reader, Ref component) {
        auto transformReader = reader.getRoot<Schema::Transform>();

        std::cout << transformReader.getRot() << "\n" << transformReader.getPos().getX() << "/" << transformReader.getPos().getY() << std::endl;
    });

    Entity e = m_world.create();
    e.add<TransformComponent>();
    auto transform = e.get<TransformComponent>();
    transform->pos = Vec2{1337, 69420};
    transform->rot = glm::pi<float>();

    m_world.saveState("./test.dat", m_world.set<TransformComponent>());
  }*/

  logInit();

  App app("Rampage", 60);
  if (app.getStatus() == Status::CriticalError)
    return -1;

  return app.run();
}
