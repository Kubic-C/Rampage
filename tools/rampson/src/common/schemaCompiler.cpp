#include "schemaCompiler.hpp"
#include <sstream>
#include <algorithm>

RAMPSON_START

// ========================================================================
//  File-local utilities
// ========================================================================

namespace {

std::string escape(const std::string& s) {
  std::string r;
  r.reserve(s.size());
  for (char c : s) {
    switch (c) {
      case '\\': r += "\\\\"; break;
      case '"':  r += "\\\""; break;
      case '\n': r += "\\n";  break;
      case '\r': r += "\\r";  break;
      case '\t': r += "\\t";  break;
      default:   r += c;      break;
    }
  }
  return r;
}

bool isPrimitive(const std::string& t) {
  return t == "int64_t" || t == "double" || t == "bool";
}

bool isRequired(const SchemaObject& obj, const std::string& name) {
  return std::find(obj.required.begin(), obj.required.end(), name) != obj.required.end();
}

} // anonymous namespace

// ========================================================================
//  toPascalCase
// ========================================================================

std::string SchemaCompiler::toPascalCase(const std::string& input) {
  std::string result;
  bool upper = true;
  for (char c : input) {
    if (c == ' ' || c == '-' || c == '_') {
      upper = true;
    } else if (upper) {
      result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      upper = false;
    } else {
      result += c;
    }
  }
  return result;
}

// ========================================================================
//  collectTypes  –  walk the tree bottom-up
// ========================================================================

void SchemaCompiler::collectTypes(const SchemaType& schema,
                                  const std::string& ctx,
                                  std::vector<ClassInfo>& out) {
  // Arrays: descend into item type
  if (schema.isArray()) {
    auto& arr = std::get<std::unique_ptr<SchemaArray>>(schema.type);
    if (arr->items && arr->items->get()) {
      collectTypes(**arr->items, ctx, out);
    }
    return;
  }

  if (!schema.isObject()) return;

  auto& obj = std::get<std::unique_ptr<SchemaObject>>(schema.type);

  // Discriminated union: recurse into oneOf variants first
  if (obj->discriminatorProperty && obj->oneOf) {
    for (const auto& variant : *obj->oneOf) {
      if (!variant->isObject()) continue;
      auto& varObj = std::get<std::unique_ptr<SchemaObject>>(variant->type);

      // Derive variant name from the discriminator const value
      std::string variantName;
      for (const auto& prop : varObj->properties) {
        if (prop.name == *obj->discriminatorProperty && prop.schema && prop.schema->isString()) {
          const auto& s = std::get<SchemaString>(prop.schema->type);
          if (s.constValue) variantName = toPascalCase(*s.constValue);
        }
      }
      if (variantName.empty()) variantName = variant->title ? toPascalCase(*variant->title) : "UnknownVariant";

      collectTypes(*variant, variantName, out);
    }
    // Add the union wrapper itself
    std::string name = schema.title ? toPascalCase(*schema.title) : toPascalCase(ctx);
    for (const auto& existing : out) {
      if (existing.name == name) return;
    }
    out.push_back({name, obj.get(), &schema});
    return;
  }

  // Recurse into properties first so dependencies are emitted before this type
  for (const auto& prop : obj->properties) {
    if (!prop.schema) continue;
    if (prop.schema->isObject()) {
      collectTypes(*prop.schema, prop.name, out);
    } else if (prop.schema->isArray()) {
      collectTypes(*prop.schema, prop.name + "Item", out);
    }
  }

  std::string name = schema.title ? toPascalCase(*schema.title) : toPascalCase(ctx);

  // Skip if a type with this name was already collected (shared sub-objects)
  for (const auto& existing : out) {
    if (existing.name == name) return;
  }
  out.push_back({name, obj.get(), &schema});
}

// ========================================================================
//  cppTypeFor  –  SchemaType → C++ type string
// ========================================================================

std::string SchemaCompiler::cppTypeFor(const SchemaType& schema, const std::string& ctx) {
  if (schema.isString())  return "std::string";
  if (schema.isNumber())  return "double";
  if (schema.isInteger()) return "int64_t";
  if(schema.isBoolean()) return "bool";

  if (schema.isArray()) {
    auto& arr = std::get<std::unique_ptr<SchemaArray>>(schema.type);
    if (arr->items && arr->items->get()) {
      return "std::vector<" + cppTypeFor(**arr->items, ctx) + ">";
    }
    return "std::vector<nlohmann::json>"; // untyped array fallback
  }

  if (schema.isObject()) {
    return schema.title ? toPascalCase(*schema.title) : toPascalCase(ctx);
  }

  return "nlohmann::json"; // should not happen for well-formed schemas
}

// ========================================================================
//  propCppType  –  convenience for ObjectProperty
// ========================================================================

std::string SchemaCompiler::propCppType(const ObjectProperty& prop) {
  if (!prop.schema) return "void";
  if (prop.schema->isArray())
    return cppTypeFor(*prop.schema, prop.name + "Item");
  return cppTypeFor(*prop.schema, prop.name);
}

// ========================================================================
//  emitTypeConstraints  –  per-property constraint metadata
// ========================================================================

void SchemaCompiler::emitTypeConstraints(std::ostream& os,
                                         const SchemaType& schema,
                                         const std::string& ind) {
  if (schema.isString()) {
    const auto& s = std::get<SchemaString>(schema.type);
    if (s.minLength)    os << ind << "static constexpr size_t minLength() { return " << *s.minLength << "; }\n";
    if (s.maxLength)    os << ind << "static constexpr size_t maxLength() { return " << *s.maxLength << "; }\n";
    if (s.pattern)      os << ind << "static const char* pattern() { return \"" << escape(*s.pattern) << "\"; }\n";
    if (s.format)       os << ind << "static const char* format() { return \"" << escape(*s.format) << "\"; }\n";
    if (s.defaultValue) os << ind << "static const char* defaultValue() { return \"" << escape(*s.defaultValue) << "\"; }\n";
    if (s.constValue)   os << ind << "static const char* constValue() { return \"" << escape(*s.constValue) << "\"; }\n";
  }
  else if (schema.isNumber()) {
    const auto& n = std::get<SchemaNumber>(schema.type);
    if (n.minimum)          os << ind << "static constexpr double minimum() { return " << *n.minimum << "; }\n";
    if (n.maximum)          os << ind << "static constexpr double maximum() { return " << *n.maximum << "; }\n";
    if (n.exclusiveMinimum) os << ind << "static constexpr double exclusiveMinimum() { return " << *n.exclusiveMinimum << "; }\n";
    if (n.exclusiveMaximum) os << ind << "static constexpr double exclusiveMaximum() { return " << *n.exclusiveMaximum << "; }\n";
    if (n.multipleOf)       os << ind << "static constexpr double multipleOf() { return " << *n.multipleOf << "; }\n";
    if (n.defaultValue)     os << ind << "static constexpr double defaultValue() { return " << *n.defaultValue << "; }\n";
    if (n.constValue)       os << ind << "static constexpr double constValue() { return " << *n.constValue << "; }\n";
  }
  else if (schema.isInteger()) {
    const auto& i = std::get<SchemaInteger>(schema.type);
    if (i.minimum)          os << ind << "static constexpr int64_t minimum() { return " << *i.minimum << "; }\n";
    if (i.maximum)          os << ind << "static constexpr int64_t maximum() { return " << *i.maximum << "; }\n";
    if (i.exclusiveMinimum) os << ind << "static constexpr int64_t exclusiveMinimum() { return " << *i.exclusiveMinimum << "; }\n";
    if (i.exclusiveMaximum) os << ind << "static constexpr int64_t exclusiveMaximum() { return " << *i.exclusiveMaximum << "; }\n";
    if (i.multipleOf)       os << ind << "static constexpr int64_t multipleOf() { return " << *i.multipleOf << "; }\n";
    if (i.defaultValue)     os << ind << "static constexpr int64_t defaultValue() { return " << *i.defaultValue << "; }\n";
    if (i.constValue)       os << ind << "static constexpr int64_t constValue() { return " << *i.constValue << "; }\n";
  }
  else if (schema.isArray()) {
    const auto& a = std::get<std::unique_ptr<SchemaArray>>(schema.type);
    if (a->minItems)    os << ind << "static constexpr size_t minItems() { return " << *a->minItems << "; }\n";
    if (a->maxItems)    os << ind << "static constexpr size_t maxItems() { return " << *a->maxItems << "; }\n";
    if (a->uniqueItems) os << ind << "static constexpr bool uniqueItems() { return " << (*a->uniqueItems ? "true" : "false") << "; }\n";
  }
}

// ========================================================================
//  emitClass  –  one complete struct definition
// ========================================================================

void SchemaCompiler::emitClass(std::ostream& os, const ClassInfo& info) {
  const auto& obj = *info.obj;
  const auto& st  = *info.schemaType;

  // Discriminated union gets its own emitter
  if (obj.discriminatorProperty && obj.oneOf) {
    emitOneOfClass(os, info);
    return;
  }

  os << "struct " << info.name << " : public JsonValue {\n";

  // ---- Class-level metadata ----
  bool hasMeta = false;
  if (st.title) {
    os << "  static const char* title() { return \"" << escape(*st.title) << "\"; }\n";
    hasMeta = true;
  }
  if (st.description) {
    os << "  static const char* description() { return \"" << escape(*st.description) << "\"; }\n";
    hasMeta = true;
  }
  if (obj.id) {
    os << "  static const char* schemaId() { return \"" << escape(*obj.id) << "\"; }\n";
    hasMeta = true;
  }
  if (hasMeta) os << "\n";

  // ---- Properties metadata struct ----
  if (!obj.properties.empty()) {
    os << "  struct Properties {\n";
    for (const auto& prop : obj.properties) {
      bool req = isRequired(obj, prop.name);
      std::string pcName = toPascalCase(prop.name);

      os << "    struct " << pcName << " {\n";
      os << "      static const char* name() { return \"" << escape(prop.name) << "\"; }\n";
      os << "      static constexpr bool required = " << (req ? "true" : "false") << ";\n";

      if (prop.schema && prop.schema->title)
        os << "      static const char* title() { return \"" << escape(*prop.schema->title) << "\"; }\n";
      if (prop.description)
        os << "      static const char* description() { return \"" << escape(*prop.description) << "\"; }\n";

      if (prop.schema)
        emitTypeConstraints(os, *prop.schema, "      ");

      os << "    };\n";
    }
    os << "  };\n\n";
  }

  // ---- Validation & deserialization ----
  emitValidate(os, info);
  emitFromJson(os, info);

  // ---- Getters & Setters ----
  for (const auto& prop : obj.properties) {
    if (!prop.schema) continue;

    bool req         = isRequired(obj, prop.name);
    std::string type = propCppType(prop);
    std::string cap  = toPascalCase(prop.name);
    std::string mem  = prop.name + "_";
    bool prim        = isPrimitive(type);

    if (req) {
      if (prim) {
        os << "  " << type << " get" << cap << "() const { return " << mem << "; }\n";
        os << "  void set" << cap << "(" << type << " value) { " << mem << " = value; }\n";
      } else {
        os << "  const " << type << "& get" << cap << "() const { return " << mem << "; }\n";
        os << "  " << type << "& get" << cap << "() { return " << mem << "; }\n";
        os << "  void set" << cap << "(const " << type << "& value) { " << mem << " = value; }\n";
      }
    } else {
      os << "  bool has" << cap << "() const { return " << mem << ".has_value(); }\n";
      if (prim) {
        os << "  " << type << " get" << cap << "() const { return " << mem << ".value(); }\n";
        os << "  void set" << cap << "(" << type << " value) { " << mem << " = value; }\n";
      } else {
        os << "  const " << type << "& get" << cap << "() const { return " << mem << ".value(); }\n";
        os << "  " << type << "& get" << cap << "() { return " << mem << ".value(); }\n";
        os << "  void set" << cap << "(const " << type << "& value) { " << mem << " = value; }\n";
      }
    }
    os << "\n";
  }

  // ---- Private members ----
  if (!obj.properties.empty()) {
    os << "private:\n";
    for (const auto& prop : obj.properties) {
      if (!prop.schema) continue;

      bool req         = isRequired(obj, prop.name);
      std::string type = propCppType(prop);
      std::string mem  = prop.name + "_";

      if (req) {
        if (isPrimitive(type))
          os << "  " << type << " " << mem << "{};\n";
        else
          os << "  " << type << " " << mem << ";\n";
      } else {
        os << "  std::optional<" << type << "> " << mem << ";\n";
      }
    }
  }

  os << "};\n\n";
}

// ========================================================================
//  compile  –  public entry point
// ========================================================================

std::string SchemaCompiler::compile(const SchemaType& schema, const std::string& sourceFile, std::string namespaceName) {
  std::ostringstream os;

  // Header
  os << "// Generated by Rampsonc, DO NOT EDIT\n";
  if (!sourceFile.empty())
    os << "// source: " << sourceFile << "\n";
  os << "// namespace: " << namespaceName << "\n";
  os << "\n#pragma once\n\n";
  os << "#include <string>\n";
  os << "#include <vector>\n";
  os << "#include <optional>\n";
  os << "#include <cstdint>\n";
  os << "#include <set>\n";
  os << "#include <regex>\n";
  os << "#include <cmath>\n";
  os << "#include <sstream>\n";
  os << "#include <stdexcept>\n";
  os << "#include <variant>\n";
  os << "#include <nlohmann/json.hpp>\n";
  os << "\n";

  os << "namespace " << namespaceName << " {\n\n";
  os << "using json = nlohmann::json;\n\n";

  // Inline rampson types with macro guard to prevent redefinition across multiple schemas
  os << "#ifndef RAMPSON_TYPES_DEFINED\n";
  os << "#define RAMPSON_TYPES_DEFINED\n\n";
  os << "struct ValidationError {\n";
  os << "  std::string path;\n";
  os << "  std::string message;\n";
  os << "};\n\n";
  os << "struct ValidationResult {\n";
  os << "  std::vector<ValidationError> errors;\n\n";
  os << "  bool valid() const { return errors.empty(); }\n\n";
  os << "  void add(const std::string& path, const std::string& message) {\n";
  os << "    errors.push_back({path, message});\n";
  os << "  }\n\n";
  os << "  void merge(const ValidationResult& other) {\n";
  os << "    errors.insert(errors.end(), other.errors.begin(), other.errors.end());\n";
  os << "  }\n\n";
  os << "  std::string summary() const {\n";
  os << "    if (errors.empty()) return \"Validation passed.\";\n";
  os << "    std::ostringstream os;\n";
  os << "    os << errors.size() << \" validation error(s):\\n\";\n";
  os << "    for (const auto& e : errors) {\n";
  os << "      os << \"  \" << e.path << \": \" << e.message << \"\\n\";\n";
  os << "    }\n";
  os << "    return os.str();\n";
  os << "  }\n";
  os << "};\n\n";
  os << "class JsonValue {\n";
  os << "public:\n";
  os << "  virtual ~JsonValue() = default;\n\n";
  os << "  template <typename T>\n";
  os << "  T* as() { return dynamic_cast<T*>(this); }\n\n";
  os << "  template <typename T>\n";
  os << "  const T* as() const { return dynamic_cast<const T*>(this); }\n\n";
  os << "  template <typename T>\n";
  os << "  bool is() const { return dynamic_cast<const T*>(this) != nullptr; }\n";
  os << "};\n\n";
  os << "#endif // RAMPSON_TYPES_DEFINED\n\n";

  // Collect all object types in dependency order (leaves first)
  std::vector<ClassInfo> types;
  collectTypes(schema, "Root", types);

  // Forward declarations
  for (const auto& t : types)
    os << "struct " << t.name << ";\n";
  if (!types.empty()) os << "\n";

  // Class definitions
  for (const auto& t : types)
    emitClass(os, t);

  os << "} // namespace " << namespaceName << "\n";

  return os.str();
}

// ========================================================================
//  emitPropertyValidation  –  validation code for one property value
// ========================================================================

void SchemaCompiler::emitPropertyValidation(std::ostream& os,
                                            const ObjectProperty& prop,
                                            const std::string& ind) {
  if (!prop.schema) return;

  std::string eName = escape(prop.name);

  os << ind << "if (j.contains(\"" << eName << "\")) {\n";
  os << ind << "  std::string fp = path.empty() ? \"" << eName
     << "\" : path + \"." << eName << "\";\n";

  // ---- string ----
  if (prop.schema->isString()) {
    const auto& s = std::get<SchemaString>(prop.schema->type);
    os << ind << "  if (!j[\"" << eName << "\"].is_string()) {\n";
    os << ind << "    result.add(fp, \"Expected string\");\n";
    os << ind << "  } else {\n";
    os << ind << "    const auto& val = j[\"" << eName
       << "\"].get_ref<const std::string&>();\n";
    if (s.minLength)
      os << ind << "    if (val.size() < " << *s.minLength
         << ") result.add(fp, \"String length below minLength "
         << *s.minLength << "\");\n";
    if (s.maxLength)
      os << ind << "    if (val.size() > " << *s.maxLength
         << ") result.add(fp, \"String length above maxLength "
         << *s.maxLength << "\");\n";
    if (s.pattern)
      os << ind << "    { static const std::regex re(\""
         << escape(*s.pattern)
         << "\"); if (!std::regex_search(val, re)) result.add(fp, "
            "\"String does not match pattern\"); }\n";
    if (s.enumValues && !s.enumValues->empty()) {
      os << ind << "    { bool ok = ";
      for (size_t i = 0; i < s.enumValues->size(); ++i) {
        if (i > 0) os << " || ";
        os << "val == \"" << escape((*s.enumValues)[i]) << "\"";
      }
      os << "; if (!ok) result.add(fp, \"Value not in enum\"); }\n";
    }
    if (s.constValue)
      os << ind << "    if (val != \"" << escape(*s.constValue)
         << "\") result.add(fp, \"Value does not match const\");\n";
    os << ind << "  }\n";
  }
  // ---- number ----
  else if (prop.schema->isNumber()) {
    const auto& n = std::get<SchemaNumber>(prop.schema->type);
    os << ind << "  if (!j[\"" << eName << "\"].is_number()) {\n";
    os << ind << "    result.add(fp, \"Expected number\");\n";
    os << ind << "  } else {\n";
    os << ind << "    double val = j[\"" << eName << "\"].get<double>();\n";
    if (n.minimum)
      os << ind << "    if (val < " << *n.minimum
         << ") result.add(fp, \"Value below minimum " << *n.minimum << "\");\n";
    if (n.maximum)
      os << ind << "    if (val > " << *n.maximum
         << ") result.add(fp, \"Value above maximum " << *n.maximum << "\");\n";
    if (n.exclusiveMinimum)
      os << ind << "    if (val <= " << *n.exclusiveMinimum
         << ") result.add(fp, \"Value at or below exclusiveMinimum\");\n";
    if (n.exclusiveMaximum)
      os << ind << "    if (val >= " << *n.exclusiveMaximum
         << ") result.add(fp, \"Value at or above exclusiveMaximum\");\n";
    if (n.multipleOf)
      os << ind << "    if (std::fmod(val, " << *n.multipleOf
         << ") != 0.0) result.add(fp, \"Value not a multiple of "
         << *n.multipleOf << "\");\n";
    if (n.constValue)
      os << ind << "    if (val != " << *n.constValue
         << ") result.add(fp, \"Value does not match const\");\n";
    if (n.enumValues && !n.enumValues->empty()) {
      os << ind << "    { bool ok = ";
      for (size_t i = 0; i < n.enumValues->size(); ++i) {
        if (i > 0) os << " || ";
        os << "val == " << (*n.enumValues)[i];
      }
      os << "; if (!ok) result.add(fp, \"Value not in enum\"); }\n";
    }
    os << ind << "  }\n";
  }
  // ---- integer ----
  else if (prop.schema->isInteger()) {
    const auto& intS = std::get<SchemaInteger>(prop.schema->type);
    os << ind << "  if (!j[\"" << eName << "\"].is_number_integer()) {\n";
    os << ind << "    result.add(fp, \"Expected integer\");\n";
    os << ind << "  } else {\n";
    os << ind << "    int64_t val = j[\"" << eName << "\"].get<int64_t>();\n";
    if (intS.minimum)
      os << ind << "    if (val < " << *intS.minimum
         << ") result.add(fp, \"Value below minimum\");\n";
    if (intS.maximum)
      os << ind << "    if (val > " << *intS.maximum
         << ") result.add(fp, \"Value above maximum\");\n";
    if (intS.exclusiveMinimum)
      os << ind << "    if (val <= " << *intS.exclusiveMinimum
         << ") result.add(fp, \"Value at or below exclusiveMinimum\");\n";
    if (intS.exclusiveMaximum)
      os << ind << "    if (val >= " << *intS.exclusiveMaximum
         << ") result.add(fp, \"Value at or above exclusiveMaximum\");\n";
    if (intS.multipleOf)
      os << ind << "    if (val % " << *intS.multipleOf
         << " != 0) result.add(fp, \"Value not a multiple of "
         << *intS.multipleOf << "\");\n";
    if (intS.constValue)
      os << ind << "    if (val != " << *intS.constValue
         << ") result.add(fp, \"Value does not match const\");\n";
    if (intS.enumValues && !intS.enumValues->empty()) {
      os << ind << "    { bool ok = ";
      for (size_t i = 0; i < intS.enumValues->size(); ++i) {
        if (i > 0) os << " || ";
        os << "val == " << (*intS.enumValues)[i];
      }
      os << "; if (!ok) result.add(fp, \"Value not in enum\"); }\n";
    }
    os << ind << "  }\n";
  }
  // ---- boolean ----
  else if (prop.schema->isBoolean()) {
    const auto& b = std::get<SchemaBoolean>(prop.schema->type);
    os << ind << "  if (!j[\"" << eName << "\"].is_boolean()) {\n";
    os << ind << "    result.add(fp, \"Expected boolean\");\n";
    os << ind << "  }";
    if (b.constValue) {
      os << " else {\n";
      os << ind << "    if (j[\"" << eName << "\"].get<bool>() != "
         << (*b.constValue ? "true" : "false")
         << ") result.add(fp, \"Value does not match const\");\n";
      os << ind << "  }\n";
    } else {
      os << "\n";
    }
  }
  // ---- nested object ----
  else if (prop.schema->isObject()) {
    std::string nestedType = cppTypeFor(*prop.schema, prop.name);
    os << ind << "  if (!j[\"" << eName << "\"].is_object()) {\n";
    os << ind << "    result.add(fp, \"Expected object\");\n";
    os << ind << "  } else {\n";
    os << ind << "    result.merge(" << nestedType << "::validate(j[\""
       << eName << "\"], fp));\n";
    os << ind << "  }\n";
  }
  // ---- array ----
  else if (prop.schema->isArray()) {
    const auto& arr = std::get<std::unique_ptr<SchemaArray>>(prop.schema->type);
    os << ind << "  if (!j[\"" << eName << "\"].is_array()) {\n";
    os << ind << "    result.add(fp, \"Expected array\");\n";
    os << ind << "  } else {\n";
    os << ind << "    const auto& arr = j[\"" << eName << "\"];\n";

    if (arr->minItems)
      os << ind << "    if (arr.size() < " << *arr->minItems
         << ") result.add(fp, \"Array size below minItems "
         << *arr->minItems << "\");\n";
    if (arr->maxItems)
      os << ind << "    if (arr.size() > " << *arr->maxItems
         << ") result.add(fp, \"Array size above maxItems "
         << *arr->maxItems << "\");\n";
    if (arr->uniqueItems && *arr->uniqueItems)
      os << ind << "    { std::set<json> seen;"
         << " for (const auto& e : arr) {"
         << " if (!seen.insert(e).second) {"
         << " result.add(fp, \"Array contains duplicate items\"); break; } } }\n";

    // Validate individual items
    if (arr->items && arr->items->get()) {
      const auto& itemSchema = **arr->items;
      if (itemSchema.isObject()) {
        std::string itemType = cppTypeFor(itemSchema, prop.name + "Item");
        os << ind << "    for (size_t i = 0; i < arr.size(); ++i) {\n";
        os << ind << "      std::string ip = fp + \"[\" + std::to_string(i) + \"]\";\n";
        os << ind << "      if (!arr[i].is_object()) result.add(ip, \"Expected object\");\n";
        os << ind << "      else result.merge(" << itemType << "::validate(arr[i], ip));\n";
        os << ind << "    }\n";
      } else if (itemSchema.isString()) {
        os << ind << "    for (size_t i = 0; i < arr.size(); ++i) {\n";
        os << ind << "      if (!arr[i].is_string()) result.add(fp + \"[\" + std::to_string(i) + \"]\", \"Expected string\");\n";
        os << ind << "    }\n";
      } else if (itemSchema.isNumber()) {
        os << ind << "    for (size_t i = 0; i < arr.size(); ++i) {\n";
        os << ind << "      if (!arr[i].is_number()) result.add(fp + \"[\" + std::to_string(i) + \"]\", \"Expected number\");\n";
        os << ind << "    }\n";
      } else if (itemSchema.isInteger()) {
        os << ind << "    for (size_t i = 0; i < arr.size(); ++i) {\n";
        os << ind << "      if (!arr[i].is_number_integer()) result.add(fp + \"[\" + std::to_string(i) + \"]\", \"Expected integer\");\n";
        os << ind << "    }\n";
      } else if (itemSchema.isBoolean()) {
        os << ind << "    for (size_t i = 0; i < arr.size(); ++i) {\n";
        os << ind << "      if (!arr[i].is_boolean()) result.add(fp + \"[\" + std::to_string(i) + \"]\", \"Expected boolean\");\n";
        os << ind << "    }\n";
      }
    }

    os << ind << "  }\n";
  }

  os << ind << "}\n";
}

// ========================================================================
//  emitValidate  –  static validate() method
// ========================================================================

void SchemaCompiler::emitValidate(std::ostream& os, const ClassInfo& info) {
  const auto& obj = *info.obj;

  os << "  static ValidationResult validate(const json& j, const std::string& path = \"\") {\n";
  os << "    ValidationResult result;\n";
  os << "    if (!j.is_object()) { result.add(path, \"Expected object\"); return result; }\n";

  // Required field checks
  for (const auto& req : obj.required) {
    std::string eReq = escape(req);
    os << "    if (!j.contains(\"" << eReq
       << "\")) result.add(path.empty() ? \"" << eReq
       << "\" : path + \"." << eReq << "\", \"Required field missing\");\n";
  }

  // Per-property validation
  for (const auto& prop : obj.properties) {
    emitPropertyValidation(os, prop, "    ");
  }

  // Additional properties check
  if (!obj.allowAdditionalProperties && !obj.properties.empty()) {
    os << "    {\n";
    os << "      static const std::set<std::string> known = {";
    for (size_t i = 0; i < obj.properties.size(); ++i) {
      if (i > 0) os << ", ";
      os << "\"" << escape(obj.properties[i].name) << "\"";
    }
    os << "};\n";
    os << "      for (auto it = j.begin(); it != j.end(); ++it) {\n";
    os << "        if (known.find(it.key()) == known.end())\n";
    os << "          result.add(path.empty() ? it.key() : path + \".\" + it.key(),"
       << " \"Additional property not allowed\");\n";
    os << "      }\n";
    os << "    }\n";
  }

  os << "    return result;\n";
  os << "  }\n\n";
}

// ========================================================================
//  emitFromJson  –  static fromJson() method
// ========================================================================

void SchemaCompiler::emitFromJson(std::ostream& os, const ClassInfo& info) {
  const auto& obj = *info.obj;

  os << "  static " << info.name << " fromJson(const json& j) {\n";
  os << "    auto vr = validate(j);\n";
  os << "    if (!vr.valid()) throw std::runtime_error(vr.summary());\n";
  os << "    " << info.name << " obj;\n";

  for (const auto& prop : obj.properties) {
    if (!prop.schema) continue;

    bool req         = isRequired(obj, prop.name);
    std::string type = propCppType(prop);
    std::string cap  = toPascalCase(prop.name);
    std::string eName = escape(prop.name);

    // Required properties use j.at(), optional use j[""] inside a contains check
    std::string jGet = req ? ("j.at(\"" + eName + "\")") : ("j[\"" + eName + "\"]");

    if (!req)
      os << "    if (j.contains(\"" << eName << "\")) {\n";

    std::string ind = req ? "    " : "      ";

    if (prop.schema->isString()) {
      os << ind << "obj.set" << cap << "(" << jGet << ".get<std::string>());\n";
    } else if (prop.schema->isNumber()) {
      os << ind << "obj.set" << cap << "(" << jGet << ".get<double>());\n";
    } else if (prop.schema->isInteger()) {
      os << ind << "obj.set" << cap << "(" << jGet << ".get<int64_t>());\n";
    } else if (prop.schema->isBoolean()) {
      os << ind << "obj.set" << cap << "(" << jGet << ".get<bool>());\n";
    } else if (prop.schema->isObject()) {
      std::string nestedType = cppTypeFor(*prop.schema, prop.name);
      os << ind << "obj.set" << cap << "(" << nestedType << "::fromJson(" << jGet << "));\n";
    } else if (prop.schema->isArray()) {
      const auto& arr = std::get<std::unique_ptr<SchemaArray>>(prop.schema->type);
      if (arr->items && arr->items->get() && (**arr->items).isObject()) {
        std::string itemType = cppTypeFor(**arr->items, prop.name + "Item");
        os << ind << "{ " << type << " items; for (const auto& e : " << jGet
           << ") items.push_back(" << itemType << "::fromJson(e)); obj.set"
           << cap << "(items); }\n";
      } else {
        os << ind << "obj.set" << cap << "(" << jGet << ".get<" << type << ">());\n";
      }
    }

    if (!req)
      os << "    }\n";
  }

  os << "    return obj;\n";
  os << "  }\n\n";
}

// ========================================================================
//  emitOneOfClass  –  discriminated union (oneOf + discriminator)
// ========================================================================

void SchemaCompiler::emitOneOfClass(std::ostream& os, const ClassInfo& info) {
  const auto& obj = *info.obj;
  const auto& st  = *info.schemaType;
  const auto& discProp = *obj.discriminatorProperty;

  // Collect variant info: discriminator const value → class name
  struct VariantInfo {
    std::string constValue;
    std::string className;
  };
  std::vector<VariantInfo> variants;

  for (const auto& variant : *obj.oneOf) {
    if (!variant->isObject()) continue;
    auto& varObj = std::get<std::unique_ptr<SchemaObject>>(variant->type);

    std::string constVal;
    for (const auto& prop : varObj->properties) {
      if (prop.name == discProp && prop.schema && prop.schema->isString()) {
        const auto& s = std::get<SchemaString>(prop.schema->type);
        if (s.constValue) constVal = *s.constValue;
      }
    }

    std::string className = variant->title ? toPascalCase(*variant->title) : toPascalCase(constVal);
    if (!className.empty())
      variants.push_back({constVal, className});
  }

  os << "struct " << info.name << " : public JsonValue {\n";

  // ---- Class-level metadata ----
  if (st.title)
    os << "  static const char* title() { return \"" << escape(*st.title) << "\"; }\n";
  if (st.description)
    os << "  static const char* description() { return \"" << escape(*st.description) << "\"; }\n";
  if (st.title || st.description) os << "\n";

  // ---- Variant typedef ----
  os << "  using Variant = std::variant<";
  for (size_t i = 0; i < variants.size(); ++i) {
    if (i > 0) os << ", ";
    os << variants[i].className;
  }
  os << ">;\n\n";

  // ---- validate ----
  std::string eDisc = escape(discProp);
  os << "  static ValidationResult validate(const json& j, const std::string& path = \"\") {\n";
  os << "    ValidationResult result;\n";
  os << "    if (!j.is_object()) { result.add(path, \"Expected object\"); return result; }\n";
  os << "    if (!j.contains(\"" << eDisc << "\")) {\n";
  os << "      result.add(path.empty() ? \"" << eDisc << "\" : path + \"." << eDisc << "\", \"Required discriminator field missing\");\n";
  os << "      return result;\n";
  os << "    }\n";
  os << "    if (!j[\"" << eDisc << "\"].is_string()) {\n";
  os << "      result.add(path.empty() ? \"" << eDisc << "\" : path + \"." << eDisc << "\", \"Discriminator must be a string\");\n";
  os << "      return result;\n";
  os << "    }\n";
  os << "    const auto& disc = j[\"" << eDisc << "\"].get_ref<const std::string&>();\n";
  for (size_t i = 0; i < variants.size(); ++i) {
    const char* prefix = (i == 0) ? "    if" : "    else if";
    os << prefix << " (disc == \"" << escape(variants[i].constValue)
       << "\") return " << variants[i].className << "::validate(j, path);\n";
  }
  os << "    else result.add(path.empty() ? \"" << eDisc << "\" : path + \"." << eDisc
     << "\", \"Unknown discriminator value '\" + disc + \"'\");\n";
  os << "    return result;\n";
  os << "  }\n\n";

  // ---- fromJson ----
  os << "  static " << info.name << " fromJson(const json& j) {\n";
  os << "    auto vr = validate(j);\n";
  os << "    if (!vr.valid()) throw std::runtime_error(vr.summary());\n";
  os << "    " << info.name << " obj;\n";
  os << "    const auto& disc = j[\"" << eDisc << "\"].get_ref<const std::string&>();\n";
  for (size_t i = 0; i < variants.size(); ++i) {
    const char* prefix = (i == 0) ? "    if" : "    else if";
    os << prefix << " (disc == \"" << escape(variants[i].constValue)
       << "\") obj.variant_ = " << variants[i].className << "::fromJson(j);\n";
  }
  os << "    return obj;\n";
  os << "  }\n\n";

  // ---- Accessors ----
  os << "  template <typename T>\n";
  os << "  bool holds() const { return std::holds_alternative<T>(variant_); }\n\n";
  os << "  template <typename T>\n";
  os << "  const T& get() const { return std::get<T>(variant_); }\n\n";
  os << "  template <typename T>\n";
  os << "  T& get() { return std::get<T>(variant_); }\n\n";
  os << "  const Variant& variant() const { return variant_; }\n";
  os << "  Variant& variant() { return variant_; }\n\n";

  // ---- Private ----
  os << "private:\n";
  os << "  Variant variant_;\n";
  os << "};\n\n";
}

RAMPSON_END
