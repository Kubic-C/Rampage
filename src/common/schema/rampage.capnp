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

struct Vec3I32 {
    x @0 :Int32;
    y @1 :Int32;
    z @2 :Int32;
}

struct TransformComponent {
    pos @0 :Vec2F32;
    rot @1 :Float32;
}

struct CameraComponent {
    zoom @0 :Float32;
    rot @1 :Float32;
}

struct ArrowComponent {
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

struct InventoryData {
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

struct ItemAttrTileComponent {
    assetId @0 :UInt32;
}

struct PlayerComponent {
    mouse @0 :Vec2F32;
    accel @1 :Float32;
    maxSpeed @2 :Float32;
}

struct CircleRenderComponent {
    radius @0 :Float32 = 0.0;
    offset @1 :Vec2F32;              # defaults to (0,0) automatically
    z @2 :Float32 = 1.0;
    color @3 :Vec3F32;               # defaults to (0,0,0) unless set
}

struct RectangleRenderComponent {
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

struct SpriteLayer {
    texIndex @0 :UInt16 = 0;
    offset @1 :Vec2F32;
    rot @2 :Float32 = 0.0;
    layer @3 :UInt8 = 255;
}

struct SubSprite {
    gridPos @0 :Vec2U16;               # grid location of this sub-sprite
    layers @1 :List(SpriteLayer);
}

struct SpriteComponent {
    scaling @0 :Float32 = 1.0;
    subSprites @1 :List(SubSprite);     # flattened list
}

struct TileItemComponent {
    item @0 :UInt32;   # EntityId of the item
}

struct MultiTileComponent {
    occupiedPositions @0 :List(Vec3I32);
    anchorPos @1 :Vec3I32;
}

struct TileComponent {
    pos @0 :Vec3I32;
    parent @1 :UInt32 = 0;
    material @2 :SurfaceMaterial;
}

struct TileEntry {
    pos @0 :Vec3I32;
    entityId @1 :UInt32;
}

struct TilemapComponent {
    tiles @0 :List(TileEntry);         # tiles map as list of key-value pairs
}

struct SurfaceMaterial {
  friction @0 :Float32;
  restitution @1 :Float32;
  rollingResistance @2 :Float32;
  tangentSpeed @3 :Float32;
  userMaterialId @4 :UInt32;
  customColor @5 :UInt32; 
}

struct Filter {
  categoryBits @0 :UInt64;
  maskBits @1 :UInt64;
  groupIndex @2 :Int16;
}

struct Circle {
  radius @0 :Float32;
}

struct Polygon {
  vertices @0 :List(Vec2F32);
  normals @1 :List(Vec2F32);
  centroid @2 :Vec2F32;
  radius @3 :Float32;
  count @4 :UInt16;
}

struct Shape {
  union {
    circle @0 :Circle;
    polygon @1 :Polygon;
  }
  def @2 :ShapeDef;
}

struct ShapeDef {
  material @0 :SurfaceMaterial;
  density @1 :Float32;
  isSensor @2 :Bool;
  enableSensorEvents @3 :Bool;
  enableContactEvents @4 :Bool;
  enableHitEvents @5 :Bool;
  enablePreSolveEvents @6 :Bool;
  invokeContactCreation @7 :Bool;
  updateBodyMass @8 :Bool;
  filter @9 :Filter;
}

enum BodyType {
  staticBody @0;
  kinematicBody @1;
  dynamicBody @2;
  invalidBody @3;
}

struct Body {
  bodyType @0 :BodyType;
  linearVelocity @1 :Vec2F32;
  angularVelocity @2 :Float32;
  linearDamping @3 :Float32;
  angularDamping @4 :Float32;
  gravityScale @5 :Float32;
  sleepThreshold @6 :Float32;
  enableSleep @7 :Bool;
  isAwake @8 :Bool;
  isBullet @9 :Bool;
  isEnabled @10 :Bool;
  allowFastRotation @11 :Bool;

  shapes @12 :List(Shape);
}

struct BodyComponent {
  body @0 :Body;
}

struct HealthComponent {
  health @0 :Float32 = 5.0;
}

struct LifetimeComponent {
  timeLeft @0 :Float32 = 1.0;
}

struct ContactDamageComponent {
  damage @0 :Float32 = 10.0;
}

struct BulletDamageComponent {
  damage @0 :Float32 = 10.0;
}

struct TurretComponent {
  summon @0 :UInt32 = 0;
  fireRate @1 :Float32 = 1.0;
  timeSinceLastShot @2 :Float32 = 0.0;
  radius @3 :Float32 = 2.0;
  rot @4 :Float32 = 0.0;
  turnSpeed @5 :Float32 = 0.1;
  shootRange @6 :Float32 = 0.04;
  stopRange @7 :Float32 = 0.02;
  muzzleVelocity @8 :Float32 = 10.0;
  bulletRadius @9 :Float32 = 0.25;
  bulletDamage @10 :Float32 = 10.0;
  bulletHealth @11 :Float32 = 1.0;
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

struct AssetEntity {
    name @0 :Text;
    id @1 :UInt32;
    compIds @2 :List(UInt16);
    compData @3 :List(Data);
}

struct ComponentIdName {
    name @0 :Text;
    compId @1 :UInt16;
}

struct State {
    registeredComponents @0 :List(ComponentIdName);
    entities @1 :List(Entity);
}

struct AssetState {
    stateName @0: Text;
    registeredComponents @1 :List(ComponentIdName);
    entities @2 :List(AssetEntity);
}

