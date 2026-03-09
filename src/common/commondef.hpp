#pragma once

/* STL */
#include <cstdint>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <memory>
#include <functional>
#include <numeric>
#include <ranges>

/* BOOST */
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

/* SDL/OpenGL */
#include <SDL3/SDL.h>
#include <glad/gl.h>

/* TGUI */
#include <TGUI/Backend/SDL-TTF-OpenGL3.hpp>
#include <TGUI/TGUI.hpp>

/* BOX2D */
#include <box2d/box2d.h>

/* OpenGL Mathematics (GLM) */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

/* JSON */
#include <nlohmann/json.hpp>

#define RAMPAGE_START namespace rmp {
#define RAMPAGE_END }

#ifndef NODISCARD
#define NODISCARD [[nodiscard]]
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#ifndef NDEBUG
#define DEBUG
#endif

RAMPAGE_START

using json = nlohmann::json;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

// Compile-time: strip "struct " or "class " prefix from type name
consteval std::string_view stripTypePrefix(std::string_view typeName) {
  if (typeName.starts_with("struct "))
    return typeName.substr(7);
  if (typeName.starts_with("class "))
    return typeName.substr(6);
  return typeName;
}

// Compile-time: strip namespace prefix (e.g., "rmp::") from type name
consteval std::string_view stripNamespace(std::string_view typeName) {
  size_t colonPos = typeName.find_last_of(':');
  if (colonPos != std::string_view::npos && colonPos + 1 < typeName.size()) {
    return typeName.substr(colonPos + 1);
  }
  return typeName;
}

// Compile-time type name extraction using __PRETTY_FUNCTION__
template <typename T>
consteval std::string_view getTypeName() {
  #ifdef _MSC_VER
    constexpr std::string_view func = __FUNCSIG__;
    // MSVC: "std::string_view __cdecl rmp::getTypeName<struct TurretComponent>(void)"
    // Find "getTypeName" function name first to avoid matching angle brackets in return type
    size_t funcNamePos = func.find("getTypeName");
    if (funcNamePos == std::string_view::npos)
      return "Unknown";
    
    // Find the template parameter start after the function name
    size_t start = func.find('<', funcNamePos);
    size_t end = func.find_last_of('>');
    if (start != std::string_view::npos && end != std::string_view::npos) {
      return stripNamespace(stripTypePrefix(func.substr(start + 1, end - start - 1)));
    }
  #else
    constexpr std::string_view func = __PRETTY_FUNCTION__;
    // GCC/Clang: "constexpr std::string_view getTypeNameCompileTime() [with T = TurretComponent]"
    size_t start = func.find('=');
    if (start != std::string_view::npos) {
      start += 2; // Skip "= "
      size_t end = func.find_first_of(";]", start);
      if (end != std::string_view::npos) {
        return stripNamespace(stripTypePrefix(func.substr(start, end - start)));
      }
    }
  #endif
  return "Unknown";
}

template <typename K, typename T>
using Map = boost::unordered_map<K, T>; /* closed addressing maps only... */

template <typename K, typename T>
using OpenMap = boost::unordered_flat_map<K, T>; /* Do not point towards any elements! */

template <typename T>
using Set = boost::unordered_flat_set<T>;

inline bool isApprox(float a, float b, float max) {
  return fabsf(a - b) < max;
}

enum class Status { CriticalError, Warning, Ok };

// https://stackoverflow.com/questions/52303316/get-index-by-type-in-stdvariant
template <typename Variant, typename T>
constexpr size_t IndexInVariant = boost::mp11::mp_find<Variant, T>::value;

RAMPAGE_END
