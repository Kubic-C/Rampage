#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include "commondef.hpp"

/* OpenGL Mathematics (GLM) */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

struct Vec2 : glm::vec2 {
  Vec2() {}

  Vec2(float scalar) : glm::vec2(scalar) {}

  Vec2(float x, float y) : glm::vec2(x, y) {}

  Vec2(const glm::vec2& other) : glm::vec2(other) {}

  Vec2(const b2Vec2& other) : glm::vec2(other.x, other.y) {}

  Vec2& operator=(const b2Vec2& other) {
    x = other.x;
    y = other.y;
    return *this;
  }

  b2Vec2 b2() const { return b2Vec2(x, y); }

  operator b2Vec2() const { return b2(); }
};

struct Rot : b2Rot {
  Rot() {
    s = b2Rot_identity.s;
    c = b2Rot_identity.c;
  }

  Rot(const b2Rot& other) {
    s = other.s;
    c = other.c;
  }

  Rot(float angle) {
    s = glm::sin(angle);
    c = glm::cos(angle);
  }

  float radians() const { return b2Rot_GetAngle(*this); }

  operator float() const { return radians(); }

  bool operator==(const b2Rot& other) const { return s == other.s && c == other.c; }

  bool operator!=(const b2Rot& other) const { return s != other.s || c != other.c; }

  Rot operator-(const Rot& other) const {
    Rot dif;
    dif.s = s * other.c - c * other.s;
    dif.c = c * other.c + s * other.s;
    return dif;
  }

  Rot operator+(const Rot& other) const {
    Rot dif;
    dif.s = s * other.c + c * other.s;
    dif.c = c * other.c - s * other.s;
    return dif;
  }
};

struct Transform {
  Vec2 pos = Vec2(0);
  Rot rot = Rot(0);

  Transform() = default;

  Transform(const Vec2& pos, const Rot& rot) : pos(pos), rot(rot) {}

  Transform(const b2Transform& other) : pos(other.p), rot(other.q) {}

  operator b2Transform() const { return b2Transform(pos.b2(), rot); }

  Transform& operator=(const b2Transform& other) {
    pos = other.p;
    rot = other.q;
    return *this;
  }

  bool operator==(const b2Transform& other) const {
    return pos.x == other.p.x && pos.y == other.p.y && rot == other.q;
  }

  bool operator!=(const b2Transform& other) const {
    return pos.x != other.p.x || pos.y != other.p.y || rot != other.q;
  }

  Vec2 getWorldPoint(const Vec2& localPos) const { return b2TransformPoint((b2Transform) * this, localPos); }

  Vec2 getLocalPoint(const Vec2& worldPos) const {
    return b2InvTransformPoint((b2Transform) * this, worldPos);
  }

  glm::mat4 matrix() const {
    glm::mat4 model = glm::identity<glm::mat4>();
    model = glm::translate(model, glm::vec3(pos, 0.0f));
    model = glm::rotate(model, rot.radians(), glm::vec3(0.0f, 0.0f, 1.0f));

    return model;
  }
};

inline bool isApprox(Vec2 value1, Vec2 value2, float max) {
  return isApprox(value1.x, value2.x, max) && isApprox(value1.y, value2.y, max);
}

inline float angleOf(const Vec2& vec) { return atan2(vec.y, vec.x); }

inline Vec2 fast2DRotate(const Vec2& vec, float angle) {
  Vec2 rotVec;

  float cs = glm::cos(angle);
  float sn = glm::sin(angle);

  rotVec.x = vec.x * cs - vec.y * sn;
  rotVec.y = vec.x * sn + vec.y * cs;

  return rotVec;
}

template <typename T>
void callInGrid(int startx, int starty, int w, int h, T&& callback) {
  int x = startx;
  int y = starty;
  while (y < h) {
    if (x > w) {
      x = startx;
      y++;
    }

    callback(x, y);
    x++;
  }
}

constexpr size_t maxNumberBits(size_t numBits) {
  size_t num = 0;

  for (size_t n = 1; n <= numBits - 1; n++) {
    size_t powerOf2 = 2;

    for (size_t power = n - 1; power > 0; power--) {
      powerOf2 *= 2;
    }

    num += powerOf2;
  }

  return num + 1;
}

template <>
struct glz::meta<glm::i16vec2> {
  using T = glm::i16vec2;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct glz::meta<glm::u16vec2> {
  using T = glm::u16vec2;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};

template <>
struct glz::meta<Vec2> {
  using T = Vec2;
  static constexpr auto value = object("x", &T::x, "y", &T::y);
};
