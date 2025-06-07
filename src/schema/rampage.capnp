@0x876ecd79dfdf8b32;
using Cxx = import "c++.capnp";
$Cxx.namespace("Schema");

struct Vec2F32 {
    x @0 :Float32;
    y @1 :Float32;
}

struct Transform {
    pos @0 :Vec2F32;
    rot @1 :Float32;
}

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