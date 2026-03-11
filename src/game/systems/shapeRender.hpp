#pragma once

#include "../components/shapes.hpp"

RAMPAGE_START

void drawRectangle(IWorldPtr world, const Transform& transform, glm::vec3 color, float hw, float hh,
                   float z);
void drawLine(IWorldPtr world, glm::vec2 from, glm::vec2 to, glm::vec3 color, float hw,
              float z = -1.0f);
void drawHollowCircle(IWorldPtr world, const Transform& transform, glm::vec3 color, float r,
                      int resolution = 30, float thickness = 0.01f, float z = -1);

bool loadShapeRender(IHost& host);

RAMPAGE_END
