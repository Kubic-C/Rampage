#pragma once

#include "world.hpp"

RAMPAGE_START

/**
 * A GameWorld is a specialized EntityWorld that represents 
 * a specific game state or level. Any entity created using this
 * derived version, will automatically be associated with the GameWorld's 
 * unique tag component. This allows for easy querying and management 
 * of game-specific entities, while still leveraging the core functionality of EntityWorld.
 */
class GameWorld {
public:

};


RAMPAGE_END