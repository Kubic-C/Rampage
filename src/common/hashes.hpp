#pragma once

#include "commondef.hpp"

// hashin' time

template <>
struct boost::hash<glm::i16vec2> {
  size_t operator()(const glm::i16vec2& pos) const noexcept {
    // Pack into a 32-bit key (since i16vec2 fits in 32 bits)
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
