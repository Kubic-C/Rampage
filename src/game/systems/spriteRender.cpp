#include "spriteRender.hpp"

#include "../../core/module.hpp"
#include "../../render/module.hpp"
#include "../../render/render.hpp"
#include "../components/sprite.hpp"
#include "../components/tilemap.hpp"
#include "../systems/tilemap.hpp"

RAMPAGE_START

struct SpriteRendererTag {};

const char* tileVertexShaderSource = R"###(
        #version 400 core
        layout(location = 0) in vec2 pos;
        layout(location = 1) in vec2 texCoords;
        layout(location = 2) in vec2 localPos;
        layout(location = 3) in vec2 worldPos;
        layout(location = 4) in uint layer;
        layout(location = 5) in float localRot;
        layout(location = 6) in float z;
        layout(location = 7) in uint dimByte;
        layout(location = 8) in float scaling;

        uniform mat4 uVP;
        uniform float uTileSideLength;

        out vec3 vTexCoords;

        vec2 rotate(vec2 vec, float angle) {
          vec2 rotVec;

          float cs = cos(angle);
          float sn = sin(angle);

          rotVec.x = vec.x * cs - vec.y * sn;
          rotVec.y = vec.x * sn + vec.y * cs;

          return rotVec;
        }

        void main() {
            vec2 dim = vec2(dimByte & 15, (dimByte & 240) >> 4);

            gl_Position = uVP * vec4(worldPos + rotate(pos * uTileSideLength * dim + localPos, localRot) * scaling, -z, 1.0);
            vTexCoords = vec3(texCoords * dim, layer);
        })###";

const char* tileFragShaderSource = R"###(
        #version 400 core
        in vec3 vTexCoords;

        uniform sampler2DArray uSampler;

        out vec4 fragColor;

        void main() {
            vec4 frag = texture(uSampler, vTexCoords);
            if (frag.a <= 0.01)
              discard;

            fragColor = frag;
        })###";

struct MeshVertex {
  glm::vec2 pos;
  glm::vec2 texCoords;
};

struct Instance {
  static constexpr u8 zMask = 0b00000111; // 7
  static constexpr u8 rotMask = 0b11111000; // 248 (31)
  static constexpr u8 dimXMask = 0b00001111; // 15
  static constexpr u8 dimYMask = 0b11110000; // 240 (15)

  glm::vec2 localPos;
  glm::vec2 worldPos;
  u16 layer;
  float rot;
  float z;
  u8 dim;
  float scale;

  void setDim(u8 dimX, u8 dimY) {
    constexpr u8 rangeLimit = 15;
    assert(dimX <= rangeLimit);
    assert(dimY <= rangeLimit);

    dim = 0;
    dim |= dimX;
    dim |= dimY << 4;
  }
};

static constexpr int instanceBufferFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

void bindInstanceBufferAttribs(VertexArrayBuffer& va, const VertexBuffer& instanceBuffer) {
  va.addVertexArrayAttrib(instanceBuffer, 2, 2, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          offsetof(Instance, localPos));
  va.addVertexArrayAttrib(instanceBuffer, 3, 2, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          offsetof(Instance, worldPos));
  va.addIntegerVertexArrayAttrib(instanceBuffer, 4, 1, GL_UNSIGNED_SHORT, sizeof(Instance),
                                 offsetof(Instance, layer));
  va.addVertexArrayAttrib(instanceBuffer, 5, 1, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          offsetof(Instance, rot));
  va.addVertexArrayAttrib(instanceBuffer, 6, 1, GL_FLOAT, GL_FALSE, sizeof(Instance), offsetof(Instance, z));
  va.addIntegerVertexArrayAttrib(instanceBuffer, 7, 1, GL_UNSIGNED_BYTE, sizeof(Instance),
                                 offsetof(Instance, dim));
  va.addVertexArrayAttrib(instanceBuffer, 8, 1, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          offsetof(Instance, scale));
  va.attribDivisor(2, 1);
  va.attribDivisor(3, 1);
  va.attribDivisor(4, 1);
  va.attribDivisor(5, 1);
  va.attribDivisor(6, 1);
  va.attribDivisor(7, 1);
  va.attribDivisor(8, 1);
}

void addSpriteInstance(Instance& newInstance, VertexArrayBuffer& va, InstanceBufferComponent& instances) {
  if (instances.count + 1 > instances.capacity) {
    instances.capacity = std::max(instances.capacity * 2, instances.count + 1);

    VertexBuffer oldBuffer = std::move(instances.instanceBuffer);
    VertexBuffer newBuffer;
    newBuffer.bufferStorage(sizeof(Instance) * instances.capacity, nullptr, instanceBufferFlags);
    oldBuffer.unmapBuffer();
    oldBuffer.bind(GL_COPY_READ_BUFFER);
    newBuffer.bind(GL_COPY_WRITE_BUFFER);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(Instance) * instances.count);
    instances.instanceBuffer = std::move(newBuffer);

    bindInstanceBufferAttribs(va, instances.instanceBuffer);
    instances.instances = static_cast<Instance*>(instances.instanceBuffer.mapBuffer(GL_WRITE_ONLY));
  }

  static_cast<Instance*>(instances.instances)[instances.count] = newInstance;
  instances.count++;
}

void meshSprite(const Transform& transform, const SpriteComponent& sprite, VertexArrayBuffer& va, InstanceBufferComponent& instances) {
  for(auto& subSpriteRow : sprite.subSprites) {
    for(auto& subSprite : subSpriteRow) {
      for (int i = 0; i < subSprite.layerCount; i++) {
        Instance instance;
        instance.localPos = subSprite.get(i).offset;
        instance.worldPos = transform.pos;
        instance.layer = subSprite.get(i).texIndex;
        instance.scale = sprite.scaling;
        instance.z = static_cast<u8>(subSprite.get(i).layer);
        instance.rot = subSprite.get(i).rot + transform.rot;
        instance.setDim(1, 1);
        addSpriteInstance(instance, va, instances);
      }
    }
  }
}

int meshSprites(IWorldPtr world, float dt) {
  EntityPtr spriteRender = world->getFirstWith(world->set<SpriteRendererTag>());
  auto va = spriteRender.get<VertexArrayBufferComponent>();
  auto instances = spriteRender.get<InstanceBufferComponent>();

  auto it = world->getWith(world->set<TransformComponent, SpriteComponent>());
  while (it->hasNext()) {
    EntityPtr entity = it->next();
    auto transform = entity.get<TransformComponent>();
    auto sprite = entity.get<SpriteComponent>();
    meshSprite(*transform, *sprite, *va, *instances);
  }

  it = world->getWith(world->set<TileComponent, SpriteComponent>());
  while (it->hasNext()) {
    EntityPtr entity = it->next();
    auto tile = entity.get<TileComponent>();
    auto sprite = entity.get<SpriteComponent>();
    if(!b2Shape_IsValid(tile->shapeId))
      continue;

    b2BodyId bodyId = b2Shape_GetBody(tile->shapeId);
    Transform transform(
      b2Body_GetWorldPoint(bodyId, TilemapComponent::getLocalTileCenter(glm::ivec2(tile->pos))),
      b2Body_GetRotation(bodyId));
    meshSprite(transform, *sprite, *va, *instances);
  }

  return 0;
}

int renderSprites(IWorldPtr world, float dt) {
  EntityPtr spriteRender = world->getFirstWith(world->set<SpriteRendererTag>());
  auto activeTextureMap = world->getFirstWith(world->set<TextureMapInUseTag>()).get<TextureMap3DComponent>();
  auto va = spriteRender.get<VertexArrayBufferComponent>();
  auto instances = spriteRender.get<InstanceBufferComponent>();
  auto shader = spriteRender.get<ShaderComponent>();

  instances->instanceBuffer.unmapBuffer();

  shader->use();
  glActiveTexture(GL_TEXTURE0);
  activeTextureMap->texArray.bind();
  activeTextureMap->sampler.bind(0);
  shader->setMat4("uVP", world->getContext<RenderModule>().getViewProj());
  va->bind();
  instances->indexBuffer.bind(GL_ELEMENT_ARRAY_BUFFER);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, instances->count);

  instances->count = 0;
  instances->instances = static_cast<Instance*>(instances->instanceBuffer.mapBuffer(GL_WRITE_ONLY));

  return 0;
}

static std::array<MeshVertex, 4> generateDefaultMesh() {
  std::array<glm::vec2, 4> tileRect = {glm::vec2(-0.5f, -0.5f), glm::vec2(0.5f, -0.5f), glm::vec2(0.5f, 0.5f),
                                       glm::vec2(-0.5f, 0.5f)};

  std::array<glm::vec2, 4> texCoords = {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f),
                                        glm::vec2(0.0f, 1.0f)};

  return {MeshVertex(tileRect[0], texCoords[0]), MeshVertex(tileRect[1], texCoords[1]),
          MeshVertex(tileRect[2], texCoords[2]), MeshVertex(tileRect[3], texCoords[3])};
}

EntityPtr createSpriteRenderEntity(IHost& host) {
  IWorldPtr world = host.getWorld();
  EntityPtr spriteRender = world->create();

  world->component<SpriteRendererTag>(false);
  spriteRender.add<SpriteRendererTag>();
  spriteRender.add<VertexArrayBufferComponent>();
  spriteRender.add<InstanceBufferComponent>();
  spriteRender.add<ShaderComponent>();

  auto va = spriteRender.get<VertexArrayBufferComponent>();
  auto instances = spriteRender.get<InstanceBufferComponent>();
  auto shader = spriteRender.get<ShaderComponent>();

  // Mesh Indicies
  std::array<u32, 6> indicies = {0, 1, 2, 2, 3, 0};
  instances->indexBuffer.bufferData(sizeof(indicies), indicies.data(), GL_STATIC_DRAW);

  // Mesh
  std::array<MeshVertex, 4> mesh = generateDefaultMesh();
  instances->meshBuffer.bufferData(sizeof(mesh), mesh.data(), GL_STATIC_DRAW);
  va->addVertexArrayAttrib(instances->meshBuffer, 0, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), 0);
  va->addVertexArrayAttrib(instances->meshBuffer, 1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                           offsetof(MeshVertex, texCoords));

  // Per Instance
  instances->capacity = 1024;
  instances->instanceBuffer.bufferStorage(sizeof(Instance) * instances->capacity, nullptr,
                                          instanceBufferFlags);
  instances->instances = instances->instanceBuffer.mapBuffer(GL_WRITE_ONLY);
  bindInstanceBufferAttribs(*va, instances->instanceBuffer);

  std::string resultStr;
  if (!shader->loadShaderStr(tileVertexShaderSource, tileFragShaderSource, resultStr)) {
    host.log("Shader Compilation Error:\n%s\n", resultStr.c_str());
    world->destroy(spriteRender);
    return EntityPtr(world, NullEntityId);
  }
  shader->use();
  shader->setInt("uSampler", 0);
  shader->setFloat("uTileSideLength", baseSpriteScale);

  return spriteRender;
}

bool loadSpriteRender(IHost& host) {
  Pipeline& pipeline = host.getPipeline();
  IWorldPtr world = host.getWorld();

  EntityPtr spriteRender = createSpriteRenderEntity(host);
  if (spriteRender.isNull())
    return false;

  // Sprite System
  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::PreRenderStage>(meshSprites);

  // Render System
  pipeline.getGroup<RenderGroup>().attachToStage<RenderGroup::OnRenderStage>(renderSprites);

  return true;
}

RAMPAGE_END
