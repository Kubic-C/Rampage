#pragma once

/* Le standard */
#include <iostream>
#include <chrono>
#include <utility>
#include <stacktrace>
#include <string_view>
#include <map>
#include <unordered_map>
#include <queue>
#include <fstream>
#include <filesystem>
#include <set>
#include <unordered_set>
#include <array>

/* box2c (optimized version of C++box2d) by erin catto */
#include <box2d/box2d.h>

/* Boost */
#include <boost/container/list.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/type_index.hpp> 
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>

/* OpenGL Mathematics (GLM) */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

/* SDL/OpenGL */
#include <glad/gl.h>
#include <SDL3/SDL.h>

#ifndef NODISCARD 
#define NODISCARD [[nodiscard]]
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

template<typename K, typename T>
using Map = boost::unordered_map<K, T>; /* closed addressing maps only... */

template<typename K, typename T>
using OpenMap = boost::unordered_flat_map<K, T>; /* Do not point towards any elements! */

template<typename T>
using Set = boost::unordered_set<T>;

inline bool isApprox(float a, float b, float max) {
  return glm::abs(a - b) < max;
}

enum class Status {
  CriticalError,
  Warning,
  Ok
};