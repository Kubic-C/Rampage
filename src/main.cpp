#include "app.hpp"

#include "ecs/serter.hpp"

int main() {
  logInit();

  {
    EntityWorldSerializable m_world;

    m_world.component<TransformComponent>();
    m_world.registerSerializable<TransformComponent>(
      [](capnp::MessageBuilder& builder, Ref component) {
        auto transformBuilder = builder.initRoot<Schema::Transform>();
        auto transform = component.cast<TransformComponent>();

        std::cout << "WRITING AS: " << "Rot:" << transform->rot << ". Pos: " << transform->pos.x << "/" << transform->pos.y << std::endl;

        transformBuilder.getPos().setX(transform->pos.x);
        transformBuilder.getPos().setY(transform->pos.y);
        transformBuilder.setRot(transform->rot);
      },
      [](capnp::MessageReader& reader, Ref component) {
        auto transformReader = reader.getRoot<Schema::Transform>();
        auto transform = component.cast<TransformComponent>();

        std::cout << "READING AS: " << "Rot:" << transformReader.getRot() << ". Pos: " << transformReader.getPos().getX() << "/" << transformReader.getPos().getY() << std::endl;

        transform->rot = transformReader.getRot();
        transform->pos.x = transformReader.getPos().getX();
        transform->pos.y = transformReader.getPos().getY();
    });

    Entity e = m_world.create();
    e.add<TransformComponent>();
    auto transform = e.get<TransformComponent>();
    transform->pos = Vec2{1337, 69420};
    transform->rot = 2.0f;

    m_world.saveState("./test.dat", m_world.set<TransformComponent>());

    m_world.destroyAllEntitiesWith(m_world.set<TransformComponent>());

    m_world.loadState("./test.dat", false);
  }

  //
  // App app("Rampage", 60);
  // if (app.getStatus() == Status::CriticalError)
  //   return -1;

  system("pause");
  // return app.run();
}
