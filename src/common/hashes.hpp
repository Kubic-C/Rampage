#pragma once

#include "commondef.hpp"

// hashin' time

template<>
struct boost::hash<glm::ivec3> {
  size_t operator()(const glm::ivec3& pos) const noexcept {
    // Pack into a 64-bit key (since ivec3 fits in 64 bits)
    uint64_t packed = (static_cast<uint64_t>(pos.x) << 32) | (static_cast<uint64_t>(pos.y) << 16) | static_cast<uint64_t>(pos.z);

    // Use a quick integer hash (e.g. a mix of Murmur finalizer)
    uint64_t h = packed;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;

    return static_cast<size_t>(h);
  }
};


template<>
struct boost::hash<glm::ivec2> {
  size_t operator()(const glm::ivec2& pos) const noexcept {
    // Pack into a 32-bit key (since ivec2 fits in 32 bits)
    uint32_t packed = (static_cast<uint16_t>(pos.x) << 16) | static_cast<uint16_t>(pos.y);

    // Use a quick integer hash (e.g. a mix of Murmur finalizer)
    uint32_t h = packed;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return static_cast<size_t>(h);
  }
};

template <>
struct boost::hash<glm::u16vec2> {
  size_t operator()(const glm::u16vec2& pos) const noexcept {
    // Pack into a 32-bit key (since u16vec2 fits in 32 bits)
    uint32_t packed = (static_cast<uint16_t>(pos.x) << 16) | static_cast<uint16_t>(pos.y);

    // Use a quick integer hash (e.g. a mix of Murmur finalizer)
    uint32_t h = packed;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return static_cast<size_t>(h);
  }
};
