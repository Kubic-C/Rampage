#include "schemaParser.hpp"

RAMPSON_START

// ========================================================================
//  Public API
// ========================================================================

std::unique_ptr<SchemaType> SchemaParser::parse(const json& schema) {
  if (!schema.is_object()) {
    throw std::runtime_error("SchemaParser::parse: root schema must be a JSON object.");
  }
  return parseSchema(schema);
}

// ========================================================================
//  Core dispatch
// ========================================================================

std::unique_ptr<SchemaType> SchemaParser::parseSchema(const json& schema) {
  if (!schema.is_object()) {
    throw std::runtime_error("SchemaParser: expected a JSON object for schema node.");
  }

  std::string type;
  if (schema.contains("type") && schema["type"].is_string()) {
    type = schema["type"].get<std::string>();
  } else {
    type = inferType(schema);
  }

  auto result = std::make_unique<SchemaType>();

  if (type == "boolean") {
    result->type = parseBoolean(schema);
  } else if (type == "string") {
    result->type = parseString(schema);
  } else if (type == "number") {
    result->type = parseNumber(schema);
  } else if (type == "integer") {
    result->type = parseInteger(schema);
  } else if (type == "array") {
    result->type = parseArray(schema);
  } else if (type == "object") {
    result->type = parseObject(schema);
  } else {
    throw std::runtime_error("SchemaParser: unsupported or unrecognised type '" + type + "'.");
  }

  parseMetadata(schema, *result);
  return result;
}

// ========================================================================
//  Type inference (when "type" is absent)
// ========================================================================

std::string SchemaParser::inferType(const json& schema) {
  if (schema.contains("properties") || schema.contains("required") ||
      schema.contains("additionalProperties") || schema.contains("allOf") ||
      schema.contains("anyOf") || schema.contains("oneOf") ||
      schema.contains("not")) {
    return "object";
  }
  if (schema.contains("items") || schema.contains("prefixItems") ||
      schema.contains("minItems") || schema.contains("maxItems") ||
      schema.contains("uniqueItems") || schema.contains("contains")) {
    return "array";
  }
  if (schema.contains("minLength") || schema.contains("maxLength") ||
      schema.contains("pattern") || schema.contains("format")) {
    return "string";
  }
  if (schema.contains("multipleOf") || schema.contains("exclusiveMinimum") ||
      schema.contains("exclusiveMaximum")) {
    // Could be number or integer – default to number.
    return "number";
  }
  if (schema.contains("minimum") || schema.contains("maximum")) {
    return "number";
  }
  // Infer type from "const" value when present
  if (schema.contains("const")) {
    const auto& cv = schema["const"];
    if (cv.is_string())  return "string";
    if (cv.is_boolean()) return "boolean";
    if (cv.is_number_integer()) return "integer";
    if (cv.is_number())  return "number";
  }
  // Fallback: treat bare schemas as objects (common for root schemas).
  return "object";
}

// ========================================================================
//  Metadata
// ========================================================================

void SchemaParser::parseMetadata(const json& schema, SchemaType& out) {
  out.title       = optionalField<std::string>(schema, "title");
  out.description = optionalField<std::string>(schema, "description");
}

// ========================================================================
//  Boolean
// ========================================================================

SchemaBoolean SchemaParser::parseBoolean(const json& schema) {
  SchemaBoolean b;
  b.constValue   = optionalField<bool>(schema, "const");
  b.defaultValue = optionalField<bool>(schema, "default");
  b.enumValues   = optionalVectorField<bool>(schema, "enum");
  b.examples     = optionalVectorField<bool>(schema, "examples");
  return b;
}

// ========================================================================
//  String
// ========================================================================

SchemaString SchemaParser::parseString(const json& schema) {
  SchemaString s;
  s.minLength   = optionalField<size_t>(schema, "minLength");
  s.maxLength   = optionalField<size_t>(schema, "maxLength");
  s.pattern     = optionalField<std::string>(schema, "pattern");
  s.format      = optionalField<std::string>(schema, "format");
  s.constValue  = optionalField<std::string>(schema, "const");
  s.defaultValue = optionalField<std::string>(schema, "default");
  s.enumValues  = optionalVectorField<std::string>(schema, "enum");
  s.examples    = optionalVectorField<std::string>(schema, "examples");
  return s;
}

// ========================================================================
//  Number
// ========================================================================

SchemaNumber SchemaParser::parseNumber(const json& schema) {
  SchemaNumber n;
  n.minimum          = optionalField<double>(schema, "minimum");
  n.maximum          = optionalField<double>(schema, "maximum");
  n.exclusiveMinimum = optionalField<double>(schema, "exclusiveMinimum");
  n.exclusiveMaximum = optionalField<double>(schema, "exclusiveMaximum");
  n.multipleOf       = optionalField<double>(schema, "multipleOf");
  n.constValue       = optionalField<double>(schema, "const");
  n.defaultValue     = optionalField<double>(schema, "default");
  n.enumValues       = optionalVectorField<double>(schema, "enum");
  n.examples         = optionalVectorField<double>(schema, "examples");
  return n;
}

// ========================================================================
//  Integer
// ========================================================================

SchemaInteger SchemaParser::parseInteger(const json& schema) {
  SchemaInteger i;
  i.minimum          = optionalField<int64_t>(schema, "minimum");
  i.maximum          = optionalField<int64_t>(schema, "maximum");
  i.exclusiveMinimum = optionalField<int64_t>(schema, "exclusiveMinimum");
  i.exclusiveMaximum = optionalField<int64_t>(schema, "exclusiveMaximum");
  i.multipleOf       = optionalField<int64_t>(schema, "multipleOf");
  i.constValue       = optionalField<int64_t>(schema, "const");
  i.defaultValue     = optionalField<int64_t>(schema, "default");
  i.enumValues       = optionalVectorField<int64_t>(schema, "enum");
  i.examples         = optionalVectorField<int64_t>(schema, "examples");
  return i;
}

// ========================================================================
//  Array
// ========================================================================

std::unique_ptr<SchemaArray> SchemaParser::parseArray(const json& schema) {
  auto arr = std::make_unique<SchemaArray>();

  arr->minItems    = optionalField<size_t>(schema, "minItems");
  arr->maxItems    = optionalField<size_t>(schema, "maxItems");
  arr->uniqueItems = optionalField<bool>(schema, "uniqueItems");

  // items – single schema applied to every element
  if (schema.contains("items") && schema["items"].is_object()) {
    arr->items = parseSchema(schema["items"]);
  }

  // prefixItems – tuple validation (array of schemas)
  if (schema.contains("prefixItems") && schema["prefixItems"].is_array()) {
    arr->prefixItems = parseSchemaArray(schema["prefixItems"]);
  }

  // contains – at least one item must match
  if (schema.contains("contains") && schema["contains"].is_object()) {
    arr->contains = parseSchema(schema["contains"]);
  }

  return arr;
}

// ========================================================================
//  Object
// ========================================================================

std::unique_ptr<SchemaObject> SchemaParser::parseObject(const json& schema) {
  auto obj = std::make_unique<SchemaObject>();

  // $id
  obj->id = optionalField<std::string>(schema, "$id");

  // additionalProperties (boolean shorthand – default true)
  if (schema.contains("additionalProperties") && schema["additionalProperties"].is_boolean()) {
    obj->allowAdditionalProperties = schema["additionalProperties"].get<bool>();
  }

  // required
  if (schema.contains("required") && schema["required"].is_array()) {
    for (const auto& r : schema["required"]) {
      if (r.is_string()) {
        obj->required.push_back(r.get<std::string>());
      }
    }
  }

  // properties
  if (schema.contains("properties") && schema["properties"].is_object()) {
    for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
      ObjectProperty prop;
      prop.name   = it.key();
      prop.schema = parseSchema(it.value());

      // Property-level description (may also be inside the sub-schema, but
      // place it on the ObjectProperty for convenience).
      if (it.value().contains("description") && it.value()["description"].is_string()) {
        prop.description = it.value()["description"].get<std::string>();
      }

      obj->properties.push_back(std::move(prop));
    }
  }

  obj->minProperties = optionalField<size_t>(schema, "minProperties");
  obj->maxProperties = optionalField<size_t>(schema, "maxProperties");

  // propertyNames – we only handle the { "enum": [...] } shorthand.
  if (schema.contains("propertyNames") && schema["propertyNames"].is_object()) {
    const auto& pn = schema["propertyNames"];
    if (pn.contains("enum") && pn["enum"].is_array()) {
      std::vector<std::string> names;
      for (const auto& v : pn["enum"]) {
        if (v.is_string()) names.push_back(v.get<std::string>());
      }
      obj->propertyNamesEnum = std::move(names);
    }
  }

  // Composition keywords
  if (schema.contains("allOf") && schema["allOf"].is_array()) {
    obj->allOf = parseSchemaArray(schema["allOf"]);
  }
  if (schema.contains("anyOf") && schema["anyOf"].is_array()) {
    obj->anyOf = parseSchemaArray(schema["anyOf"]);
  }
  if (schema.contains("oneOf") && schema["oneOf"].is_array()) {
    obj->oneOf = parseSchemaArray(schema["oneOf"]);
  }

  // discriminator
  if (schema.contains("discriminator") && schema["discriminator"].is_object()) {
    obj->discriminatorProperty = optionalField<std::string>(schema["discriminator"], "propertyName");
  }

  // "not"
  if (schema.contains("not") && schema["not"].is_object()) {
    obj->notSchema = parseSchema(schema["not"]);
  }

  return obj;
}

// ========================================================================
//  Helpers
// ========================================================================

std::vector<std::unique_ptr<SchemaType>> SchemaParser::parseSchemaArray(const json& arr) {
  std::vector<std::unique_ptr<SchemaType>> result;
  result.reserve(arr.size());
  for (const auto& element : arr) {
    result.push_back(parseSchema(element));
  }
  return result;
}

// ---- Generic optional field extraction ------------------------------------

template <typename T>
std::optional<T> SchemaParser::optionalField(const json& obj, const char* key) {
  if (obj.contains(key)) {
    try {
      return obj[key].get<T>();
    } catch (...) {
      // Type mismatch – treat as absent.
    }
  }
  return std::nullopt;
}

template <typename T>
std::optional<std::vector<T>> SchemaParser::optionalVectorField(const json& obj, const char* key) {
  if (obj.contains(key) && obj[key].is_array()) {
    std::vector<T> vec;
    vec.reserve(obj[key].size());
    for (const auto& element : obj[key]) {
      try {
        vec.push_back(element.get<T>());
      } catch (...) {
        // Skip elements that don't convert.
      }
    }
    return vec;
  }
  return std::nullopt;
}

// Explicit template instantiations for the types we use.
template std::optional<std::string>  SchemaParser::optionalField<std::string>(const json&, const char*);
template std::optional<double>       SchemaParser::optionalField<double>(const json&, const char*);
template std::optional<int64_t>      SchemaParser::optionalField<int64_t>(const json&, const char*);
template std::optional<size_t>       SchemaParser::optionalField<size_t>(const json&, const char*);
template std::optional<bool>         SchemaParser::optionalField<bool>(const json&, const char*);

template std::optional<std::vector<std::string>> SchemaParser::optionalVectorField<std::string>(const json&, const char*);
template std::optional<std::vector<double>>      SchemaParser::optionalVectorField<double>(const json&, const char*);
template std::optional<std::vector<int64_t>>     SchemaParser::optionalVectorField<int64_t>(const json&, const char*);
template std::optional<std::vector<bool>>        SchemaParser::optionalVectorField<bool>(const json&, const char*);

RAMPSON_END
