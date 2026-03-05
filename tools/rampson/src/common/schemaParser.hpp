#pragma once

#include "schema.hpp"
#include <string>
#include <stdexcept>

RAMPSON_START

// Parses a JSON Schema document (draft 2020-12 compatible) into the SchemaType
// representation defined in schema.hpp.
//
// Usage:
//   auto schema = SchemaParser::parse(jsonSchemaDoc);
//
// Throws std::runtime_error on invalid or unsupported schema input.
class SchemaParser {
public:
  // Parse the root JSON Schema document. Returns the top-level SchemaType.
  static std::unique_ptr<SchemaType> parse(const json& schema);

private:
  // --------------- Core dispatch ---------------
  static std::unique_ptr<SchemaType> parseSchema(const json& schema);

  // --------------- Type-specific parsers ---------------
  static SchemaBoolean  parseBoolean(const json& schema);
  static SchemaString   parseString(const json& schema);
  static SchemaNumber   parseNumber(const json& schema);
  static SchemaInteger  parseInteger(const json& schema);
  static std::unique_ptr<SchemaArray>  parseArray(const json& schema);
  static std::unique_ptr<SchemaObject> parseObject(const json& schema);

  // --------------- Metadata ---------------
  static void parseMetadata(const json& schema, SchemaType& out);

  // --------------- Helpers ---------------
  // Try to infer the type when the "type" field is absent.
  static std::string inferType(const json& schema);

  // Parse a list of SchemaType from a JSON array (used by allOf, anyOf, oneOf, prefixItems).
  static std::vector<std::unique_ptr<SchemaType>> parseSchemaArray(const json& arr);

  // Safe accessors
  template <typename T>
  static std::optional<T> optionalField(const json& obj, const char* key);

  template <typename T>
  static std::optional<std::vector<T>> optionalVectorField(const json& obj, const char* key);
};

RAMPSON_END
