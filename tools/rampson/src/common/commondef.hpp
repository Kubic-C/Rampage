#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <optional>
#include <variant>

#define RAMPSON_START namespace rmp::json {
#define RAMPSON_END }

RAMPSON_START

using nlohmann::json;

RAMPSON_END