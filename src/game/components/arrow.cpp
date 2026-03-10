#include "arrow.hpp"

RAMPAGE_START

void ArrowComponent::serialize(capnp::MessageBuilder& builder, Ref component) {
    auto arrowBuilder = builder.initRoot<Schema::ArrowComponent>();
    auto arrow = component.cast<ArrowComponent>();

    // Serialize dir
    arrowBuilder.getDir().setX(arrow->dir.x);
    arrowBuilder.getDir().setY(arrow->dir.y);

    // Serialize cost
    arrowBuilder.setCost(arrow->cost);

    // Serialize generation
    arrowBuilder.setGeneration(arrow->gen);

    // Serialize tileCost
    arrowBuilder.setTileCost(arrow->tileCost);
}

void ArrowComponent::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
    auto arrowReader = reader.getRoot<Schema::ArrowComponent>();
    auto arrow = component.cast<ArrowComponent>();

    // Deserialize dir
    arrow->dir.x = arrowReader.getDir().getX();
    arrow->dir.y = arrowReader.getDir().getY();

    // Deserialize cost
    arrow->cost = arrowReader.getCost();

    // Deserialize generation
    arrow->gen = arrowReader.getGeneration();

    // Deserialize tileCost
    arrow->tileCost = arrowReader.getTileCost();
}

void ArrowComponent::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
    auto arrow = component.cast<ArrowComponent>();
    auto compJson = jsonValue.as<JSchema::ArrowComponent>();

    // Parse tileCost
    if(compJson->hasTileCost())
        arrow->tileCost = compJson->getTileCost();
}

void VectorTilemapPathfinding::serialize(capnp::MessageBuilder& builder, Ref component) {
    auto pathBuilder = builder.initRoot<Schema::VectorTilemapPathfinding>();
    auto self = component.cast<VectorTilemapPathfinding>();

    // Serialize oldTarget
    auto oldTargetBuilder = pathBuilder.getOldTarget();
    oldTargetBuilder.setX(self->oldTarget.x);
    oldTargetBuilder.setY(self->oldTarget.y);

    // Serialize curGen
    pathBuilder.setCurGen(self->curGen);

    // Serialize nodes
    auto nodesBuilder = pathBuilder.initNodes((u32)self->nodes.size());
    u32 i = 0;
    for (const auto& [pos, node] : self->nodes) {
        auto nodeBuilder = nodesBuilder[i++];
        auto posBuilder = nodeBuilder.getPos();
        posBuilder.setX(pos.x);
        posBuilder.setY(pos.y);
        auto dirBuilder = nodeBuilder.getDir();
        dirBuilder.setX(node.dir.x);
        dirBuilder.setY(node.dir.y);
        nodeBuilder.setCost(node.cost);
        nodeBuilder.setGen(node.gen);
    }
}

void VectorTilemapPathfinding::deserialize(capnp::MessageReader& reader, const IdMapper& id, Ref component) {
    auto pathReader = reader.getRoot<Schema::VectorTilemapPathfinding>();
    auto self = component.cast<VectorTilemapPathfinding>();

    // Deserialize oldTarget
    auto oldTargetReader = pathReader.getOldTarget();
    self->oldTarget.x = oldTargetReader.getX();
    self->oldTarget.y = oldTargetReader.getY();

    // Deserialize curGen
    self->curGen = pathReader.getCurGen();

    // Deserialize nodes
    self->nodes.clear();
    const auto nodesReader = pathReader.getNodes();
    for (auto nodeReader : nodesReader) {
        glm::ivec2 pos(nodeReader.getPos().getX(), nodeReader.getPos().getY());
        Node node;
        node.dir.x = nodeReader.getDir().getX();
        node.dir.y = nodeReader.getDir().getY();
        node.cost = nodeReader.getCost();
        node.gen = nodeReader.getGen();
        self->nodes[pos] = node;
    }
}

void VectorTilemapPathfinding::fromJson(Ref component, AssetLoader loader, const JSchema::JsonValue& jsonValue) {
    auto self = component.cast<VectorTilemapPathfinding>();
    auto compJson = jsonValue.as<JSchema::VectorTilemapPathfinding>();

    // Parse oldTarget
    if(compJson->hasOldTarget()) {
        auto oldTargetJson = compJson->getOldTarget();
        if(oldTargetJson.hasX()) self->oldTarget.x = oldTargetJson.getX();
        if(oldTargetJson.hasY()) self->oldTarget.y = oldTargetJson.getY();
    }

    // Parse curGen
    if(compJson->hasCurGen())
        self->curGen = compJson->getCurGen();

    // Parse nodes
    if(compJson->hasNodes()) {
        self->nodes.clear();
        for(const auto& nodeJson : compJson->getNodes()) {
            glm::ivec2 pos(0);
            if(nodeJson.hasTilePos()) {
                auto posJson = nodeJson.getTilePos();
                if(posJson.hasX()) pos.x = posJson.getX();
                if(posJson.hasY()) pos.y = posJson.getY();
            }
            Node node;
            if(nodeJson.hasDir()) {
                auto dirJson = nodeJson.getDir();
                if(dirJson.hasX()) node.dir.x = dirJson.getX();
                if(dirJson.hasY()) node.dir.y = dirJson.getY();
            }
            if(nodeJson.hasCost()) node.cost = nodeJson.getCost();
            if(nodeJson.hasGen()) node.gen = nodeJson.getGen();
            self->nodes[pos] = node;
        }
    }
}

RAMPAGE_END
