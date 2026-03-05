#pragma once

#include "commondef.hpp"
#include <string>
#include <sstream>


RAMPSON_START

enum class JsonTypeName {
  Null,
  Boolean,
  Object,
  Array,
  Number,
  String,
  Integer,
  Unknown
};

// ------------------- Primitive Types -------------------

struct SchemaBoolean {
  std::optional<bool> constValue;         // "const" keyword
  std::optional<bool> defaultValue;
  std::optional<std::vector<bool>> examples;
  std::optional<std::vector<bool>> enumValues; // allowed boolean values (true/false)
};

struct SchemaString {
  std::optional<size_t> minLength;
  std::optional<size_t> maxLength;
  std::optional<std::string> pattern;            // regex pattern
  std::optional<std::string> format;             // e.g., "email", "date-time"
  std::optional<std::vector<std::string>> enumValues; // allowed string values
  std::optional<std::string> constValue;         // "const" keyword
  std::optional<std::string> defaultValue;
  std::optional<std::vector<std::string>> examples;
};

struct SchemaNumber {
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::optional<double> exclusiveMinimum;
  std::optional<double> exclusiveMaximum;
  std::optional<double> multipleOf;             // must be a multiple of this value
  std::optional<double> constValue;
  std::optional<double> defaultValue;
  std::optional<std::vector<double>> enumValues; // allowed numeric values
  std::optional<std::vector<double>> examples;
};

struct SchemaInteger {
  std::optional<int64_t> minimum;
  std::optional<int64_t> maximum;
  std::optional<int64_t> exclusiveMinimum;
  std::optional<int64_t> exclusiveMaximum;
  std::optional<int64_t> multipleOf;           // must be a multiple of this value
  std::optional<int64_t> constValue;
  std::optional<int64_t> defaultValue;
  std::optional<std::vector<int64_t>> enumValues; // allowed integer values
  std::optional<std::vector<int64_t>> examples;
};

// ------------------- SchemaType (Variant Wrapper) -------------------

struct SchemaArray;
struct SchemaObject;

struct SchemaType {
  // The actual type
  std::variant<
      SchemaBoolean,
      SchemaString,
      SchemaNumber,
      SchemaInteger,
      std::unique_ptr<SchemaArray>,
      std::unique_ptr<SchemaObject>
  > type;

  // Optional common metadata
  std::optional<std::string> title;
  std::optional<std::string> description;

  // Helper functions
  bool isBoolean() const { return std::holds_alternative<SchemaBoolean>(type); }
  bool isString() const { return std::holds_alternative<SchemaString>(type); }
  bool isNumber() const { return std::holds_alternative<SchemaNumber>(type); }
  bool isInteger() const { return std::holds_alternative<SchemaInteger>(type); }
  bool isArray() const { return std::holds_alternative<std::unique_ptr<SchemaArray>>(type); }
  bool isObject() const { return std::holds_alternative<std::unique_ptr<SchemaObject>>(type); }
};

// ------------------- Array -------------------

struct SchemaArray {
  std::optional<size_t> minItems;
  std::optional<size_t> maxItems;
  std::optional<bool> uniqueItems;              // if true, all items must be unique
  std::optional<std::unique_ptr<SchemaType>> items; // schema for items in the array
  std::optional<std::vector<std::unique_ptr<SchemaType>>> prefixItems; // tuple validation
  std::optional<std::unique_ptr<SchemaType>> contains; // must contain at least one item matching
  std::optional<std::vector<SchemaType>> defaultValue; // default array
  std::optional<std::vector<std::vector<SchemaType>>> examples;
};

// ------------------- Object Property -------------------

struct ObjectProperty {
  std::string name;                       // property key
  std::optional<std::string> description; // property description (optional)
  std::unique_ptr<SchemaType> schema;     // schema for this property
};

// ------------------- Object -------------------

struct SchemaObject {
  std::optional<std::string> id;                  // $id field
  bool allowAdditionalProperties = true;          // additionalProperties rule (default true)
  std::vector<std::string> required;             // required property names
  std::vector<ObjectProperty> properties;        // object properties
  std::optional<size_t> minProperties;
  std::optional<size_t> maxProperties;
  std::optional<std::vector<std::string>> propertyNamesEnum; // allowed property names
  std::optional<std::vector<std::unique_ptr<SchemaType>>> allOf; // composition
  std::optional<std::vector<std::unique_ptr<SchemaType>>> anyOf;
  std::optional<std::vector<std::unique_ptr<SchemaType>>> oneOf;
  std::optional<std::string> discriminatorProperty; // from "discriminator": { "propertyName": "..." }
  std::optional<std::unique_ptr<SchemaType>> notSchema;       // "not" keyword
};

// ------------------- Print -------------------

// Recursively prints a SchemaType tree to a string.
// indent controls the initial indentation level (number of spaces).
std::string print(const SchemaType& schema, int indent = 0);
std::string print(const SchemaArray& schema, int indent = 0);
std::string print(const SchemaObject& schema, int indent = 0);

RAMPSON_END