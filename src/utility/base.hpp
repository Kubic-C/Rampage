#pragma once

/* Le standard */
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stacktrace>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

/* box2c (optimized version of C++box2d) by erin catto */
#include <box2d/box2d.h>

/* Boost */
#include <boost/container/flat_map.hpp>
#include <boost/container/list.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/type_index.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

/* OpenGL Mathematics (GLM) */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

/* SDL/OpenGL */
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>

/* TGUI */
#include <TGUI/Backend/SDL-TTF-OpenGL3.hpp>
#include <TGUI/TGUI.hpp>

/* JSOOOOOOOOOOOOOOOOOOOON */
#include <glaze/glaze.hpp>

#ifndef NODISCARD
#define NODISCARD [[nodiscard]]
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

template <typename K, typename T>
using Map = boost::unordered_map<K, T>; /* closed addressing maps only... */

template <typename K, typename T>
using OpenMap = boost::unordered_flat_map<K, T>; /* Do not point towards any elements! */

template <typename T>
using Set = boost::unordered_set<T>;

inline bool isApprox(float a, float b, float max) { return glm::abs(a - b) < max; }

enum class Status { CriticalError, Warning, Ok };

// gets the filename, excluding the file extension: file.*
inline std::string getFilename(const std::string& path) {
  size_t lastOfSlash = 0;
  {
    size_t forwardSlash = path.find_last_of('/');
    size_t backSlash = path.find_last_of('\\');
    lastOfSlash =
        std::max(forwardSlash == SIZE_MAX ? 0 : forwardSlash + 1, backSlash == SIZE_MAX ? 0 : backSlash + 1);
  }
  size_t lastOfDot = path.find_last_of('.');

  if (lastOfDot == SIZE_MAX)
    lastOfDot = path.size();
  if (lastOfSlash == 0)
    return path.substr(0, lastOfDot);
  else
    return path.substr(lastOfSlash, lastOfDot - lastOfSlash);
}

// gets the directory a file is in
inline std::string getPath(const std::string& path) {
  size_t lastOfSlash = 0;
  {
    size_t forwardSlash = path.find_last_of('/');
    size_t backSlash = path.find_last_of('\\');
    lastOfSlash =
        std::max(forwardSlash == SIZE_MAX ? 0 : forwardSlash + 1, backSlash == SIZE_MAX ? 0 : backSlash + 1);
  }

  return path.substr(0, lastOfSlash);
}

// https://stackoverflow.com/questions/52303316/get-index-by-type-in-stdvariant

template <typename Variant, typename T>
constexpr size_t IndexInVariant = boost::mp11::mp_find<Variant, T>::value;
