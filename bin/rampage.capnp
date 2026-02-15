@0x876ecd79dfdf8b32;
using Cxx = import "c++.capnp";
$Cxx.namespace("Schema");

struct Vec2F32 {
    x @0 :Float32;
    y @1 :Float32;
}

struct Vec3F32 {
    x @0 :Float32;
    y @1 :Float32;
    z @2 :Float32;
}

struct Vec2U16 {
    x @0 :UInt16;
    y @1 :UInt16;
}

struct Vec2I16 {
    x @0 :Int16;
    y @1 :Int16;
}

struct Transform {
    pos @0 :Vec2F32;
    rot @1 :Float32;
}

struct Camera {
    zoom @0 :Float32;
    rot @1 :Float32;
}

struct Arrow {
    dir @0 :Vec2F32;
    cost @1 :Float32;
    generation @2 :UInt32;
    tileCost @3 :UInt32;
}

struct ItemStack {
    location @0: Vec2U16;
    maxStackCost @1 :UInt32;
    stackCount @2 :Int32;
    itemEntityId @3 :UInt32;
}

struct Inventory {
    name @0 :Text;
    rows @1 :UInt16;
    cols @2 :UInt16;
    items @3 :List(ItemStack);
    visible @4 :Bool;
}

struct ItemAttrIcon {
    name @0 :Text;
}

struct ItemAttrStackCost {
    stackCost @0 :UInt8;
}

struct ItemAttrTile {
    assetId @0 :UInt32;
}

struct Player {
    mouse @0 :Vec2F32;
    accel @1 :Float32;
    maxSpeed @2 :Float32;
}

struct CircleRender {
    radius @0 :Float32 = 0.0;
    offset @1 :Vec2F32;              # defaults to (0,0) automatically
    z @2 :Float32 = 1.0;
    color @3 :Vec3F32;               # defaults to (0,0,0) unless set
}

struct RectangleRender {
    hw @0 :Float32 = 0.0;
    hh @1 :Float32 = 0.0;
    z @2 :Float32 = 1.0;
    color @3 :Vec3F32;
}

struct SpawnerComponent {
    spawn @0 :UInt32;
    spawnRate @1 :Float32 = 1.0;
    timeSinceLastSpawn @2 :Float32 = 0.0;
    spawnableRadius @3 :Float32 = 5.0;
    spawnCount @4 :UInt32 = 5;
}

enum WorldLayer {
    top @0;
    res2 @1;
    res @2;
    item @3;
    turret @4;
    wall @5;
    floor @6;
    bottom @7;
    invalid @8;
}

struct SpriteLayer {
    texIndex @0 :UInt16 = 0;
    offset @1 :Vec2F32;
    rot @2 :Float32 = 0.0;
    layer @3 :WorldLayer = invalid;
}

struct SubSprite {
    gridPos @0 :Vec2U16;               # grid location of this sub-sprite
    layers @1 :List(SpriteLayer);
}

struct SpriteComponent {
    scaling @0 :Float32 = 1.0;
    subSprites @1 :List(SubSprite);     # flattened list
}

struct TileBoundComponent {
    layer @0 :UInt32; 
    pos @1 :Vec2I16;
    parent @2 :UInt32;
}

struct TileItemComponent {
    item @0 :UInt32;   # EntityId of the item
}

struct TileDef {
    enableCollision @0 :Bool = false;
    entity @1 :UInt32 = 0;
    sizeX @2 :UInt16 = 1;
    sizeY @3 :UInt16 = 1;
}

struct Tile {
    entity @0 :UInt32 = 0;       # Entity occupying this tile
    flags @1 :UInt8 = 0;         # TileFlags bitfield

    # Grid position of main tile
    gridPos @2 :Vec2I16;

    # Size (width, height) for multi-tile
    sizeX @3 :UInt16 = 1;
    sizeY @4 :UInt16 = 1;
}

struct TilemapComponent {
    layerCount @0 :UInt32 = 2;    # default layer count
    tiles @1 :List(Tile);         # flattened list of all tiles across layers
}

# Meant for tags.
struct Void {}

# This is not the most optimal way to store entity data,
# for now it will work as basic save data.
struct Entity {
    id @0 :UInt32;
    compIds @1 :List(UInt16);
    # a list of lists, where index of the upper list correlates to the compId located in the in above
    compData @2 :List(Data);
}

struct ComponentIdName {
    name @0 :Text;
    compId @1 :UInt16;
}

struct State {
    registeredComponents @0 :List(ComponentIdName);
    entities @1 :List(Entity);
}