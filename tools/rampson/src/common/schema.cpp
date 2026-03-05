#include "schema.hpp"
#include <sstream>

RAMPSON_START

// ========================================================================
//  Indentation helper
// ========================================================================

static std::string pad(int indent) {
  return std::string(static_cast<size_t>(indent), ' ');
}

// ========================================================================
//  Primitive printers
// ========================================================================

static void printString(std::ostringstream& os, const SchemaString& s, int indent) {
  std::string p = pad(indent);
  os << p << "[string]\n";
  if (s.minLength)   os << p << "  minLength: "   << *s.minLength   << "\n";
  if (s.maxLength)   os << p << "  maxLength: "   << *s.maxLength   << "\n";
  if (s.pattern)     os << p << "  pattern: \""    << *s.pattern     << "\"\n";
  if (s.format)      os << p << "  format: \""     << *s.format      << "\"\n";
  if (s.constValue)  os << p << "  const: \""      << *s.constValue  << "\"\n";
  if (s.defaultValue)os << p << "  default: \""    << *s.defaultValue<< "\"\n";
  if (s.enumValues) {
    os << p << "  enum: [";
    for (size_t i = 0; i < s.enumValues->size(); ++i) {
      if (i) os << ", ";
      os << "\"" << (*s.enumValues)[i] << "\"";
    }
    os << "]\n";
  }
}

static void printNumber(std::ostringstream& os, const SchemaNumber& n, int indent) {
  std::string p = pad(indent);
  os << p << "[number]\n";
  if (n.minimum)          os << p << "  minimum: "          << *n.minimum          << "\n";
  if (n.maximum)          os << p << "  maximum: "          << *n.maximum          << "\n";
  if (n.exclusiveMinimum) os << p << "  exclusiveMinimum: " << *n.exclusiveMinimum << "\n";
  if (n.exclusiveMaximum) os << p << "  exclusiveMaximum: " << *n.exclusiveMaximum << "\n";
  if (n.multipleOf)       os << p << "  multipleOf: "       << *n.multipleOf       << "\n";
  if (n.constValue)       os << p << "  const: "            << *n.constValue       << "\n";
  if (n.defaultValue)     os << p << "  default: "          << *n.defaultValue     << "\n";
  if (n.enumValues) {
    os << p << "  enum: [";
    for (size_t i = 0; i < n.enumValues->size(); ++i) {
      if (i) os << ", ";
      os << (*n.enumValues)[i];
    }
    os << "]\n";
  }
}

static void printInteger(std::ostringstream& os, const SchemaInteger& n, int indent) {
  std::string p = pad(indent);
  os << p << "[integer]\n";
  if (n.minimum)          os << p << "  minimum: "          << *n.minimum          << "\n";
  if (n.maximum)          os << p << "  maximum: "          << *n.maximum          << "\n";
  if (n.exclusiveMinimum) os << p << "  exclusiveMinimum: " << *n.exclusiveMinimum << "\n";
  if (n.exclusiveMaximum) os << p << "  exclusiveMaximum: " << *n.exclusiveMaximum << "\n";
  if (n.multipleOf)       os << p << "  multipleOf: "       << *n.multipleOf       << "\n";
  if (n.constValue)       os << p << "  const: "            << *n.constValue       << "\n";
  if (n.defaultValue)     os << p << "  default: "          << *n.defaultValue     << "\n";
  if (n.enumValues) {
    os << p << "  enum: [";
    for (size_t i = 0; i < n.enumValues->size(); ++i) {
      if (i) os << ", ";
      os << (*n.enumValues)[i];
    }
    os << "]\n";
  }
}

// ========================================================================
//  Forward declaration for mutual recursion
// ========================================================================

static void printSchemaType(std::ostringstream& os, const SchemaType& schema, int indent);
static void printSchemaArray(std::ostringstream& os, const SchemaArray& arr, int indent);
static void printSchemaObject(std::ostringstream& os, const SchemaObject& obj, int indent);

// ========================================================================
//  Helper to print a vector of SchemaType pointers (allOf, anyOf, oneOf…)
// ========================================================================

static void printSchemaList(std::ostringstream& os, const std::vector<std::unique_ptr<SchemaType>>& list,
                            const char* label, int indent) {
  std::string p = pad(indent);
  os << p << label << ":\n";
  for (size_t i = 0; i < list.size(); ++i) {
    os << p << "  [" << i << "]:\n";
    printSchemaType(os, *list[i], indent + 4);
  }
}

// ========================================================================
//  SchemaArray
// ========================================================================

static void printSchemaArray(std::ostringstream& os, const SchemaArray& arr, int indent) {
  std::string p = pad(indent);
  os << p << "[array]\n";
  if (arr.minItems)    os << p << "  minItems: "    << *arr.minItems    << "\n";
  if (arr.maxItems)    os << p << "  maxItems: "    << *arr.maxItems    << "\n";
  if (arr.uniqueItems) os << p << "  uniqueItems: " << (*arr.uniqueItems ? "true" : "false") << "\n";

  if (arr.items && arr.items->get()) {
    os << p << "  items:\n";
    printSchemaType(os, **arr.items, indent + 4);
  }

  if (arr.prefixItems) {
    os << p << "  prefixItems:\n";
    for (size_t i = 0; i < arr.prefixItems->size(); ++i) {
      os << p << "    [" << i << "]:\n";
      printSchemaType(os, *(*arr.prefixItems)[i], indent + 6);
    }
  }

  if (arr.contains && arr.contains->get()) {
    os << p << "  contains:\n";
    printSchemaType(os, **arr.contains, indent + 4);
  }
}

// ========================================================================
//  SchemaObject
// ========================================================================

static void printSchemaObject(std::ostringstream& os, const SchemaObject& obj, int indent) {
  std::string p = pad(indent);
  os << p << "[object]\n";

  if (obj.id) os << p << "  $id: \"" << *obj.id << "\"\n";
  os << p << "  additionalProperties: " << (obj.allowAdditionalProperties ? "true" : "false") << "\n";

  if (!obj.required.empty()) {
    os << p << "  required: [";
    for (size_t i = 0; i < obj.required.size(); ++i) {
      if (i) os << ", ";
      os << "\"" << obj.required[i] << "\"";
    }
    os << "]\n";
  }

  if (obj.minProperties) os << p << "  minProperties: " << *obj.minProperties << "\n";
  if (obj.maxProperties) os << p << "  maxProperties: " << *obj.maxProperties << "\n";

  if (obj.propertyNamesEnum) {
    os << p << "  propertyNames.enum: [";
    for (size_t i = 0; i < obj.propertyNamesEnum->size(); ++i) {
      if (i) os << ", ";
      os << "\"" << (*obj.propertyNamesEnum)[i] << "\"";
    }
    os << "]\n";
  }

  if (!obj.properties.empty()) {
    os << p << "  properties:\n";
    for (const auto& prop : obj.properties) {
      os << p << "    \"" << prop.name << "\"";
      if (prop.description) os << "  // " << *prop.description;
      os << ":\n";
      if (prop.schema) {
        printSchemaType(os, *prop.schema, indent + 6);
      }
    }
  }

  if (obj.allOf) printSchemaList(os, *obj.allOf, "  allOf", indent);
  if (obj.anyOf) printSchemaList(os, *obj.anyOf, "  anyOf", indent);
  if (obj.oneOf) printSchemaList(os, *obj.oneOf, "  oneOf", indent);

  if (obj.notSchema && obj.notSchema->get()) {
    os << p << "  not:\n";
    printSchemaType(os, **obj.notSchema, indent + 4);
  }
}

// ========================================================================
//  SchemaType (dispatch)
// ========================================================================

static void printSchemaType(std::ostringstream& os, const SchemaType& schema, int indent) {
  std::string p = pad(indent);

  // Metadata
  if (schema.title)       os << p << "title: \""       << *schema.title       << "\"\n";
  if (schema.description) os << p << "description: \"" << *schema.description << "\"\n";

  // Dispatch on variant
  std::visit([&](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, SchemaString>) {
      printString(os, arg, indent);
    } else if constexpr (std::is_same_v<T, SchemaNumber>) {
      printNumber(os, arg, indent);
    } else if constexpr (std::is_same_v<T, SchemaInteger>) {
      printInteger(os, arg, indent);
    } else if constexpr (std::is_same_v<T, std::unique_ptr<SchemaArray>>) {
      if (arg) printSchemaArray(os, *arg, indent);
    } else if constexpr (std::is_same_v<T, std::unique_ptr<SchemaObject>>) {
      if (arg) printSchemaObject(os, *arg, indent);
    }
  }, schema.type);
}

// ========================================================================
//  Public print() overloads
// ========================================================================

std::string print(const SchemaType& schema, int indent) {
  std::ostringstream os;
  printSchemaType(os, schema, indent);
  return os.str();
}

std::string print(const SchemaArray& schema, int indent) {
  std::ostringstream os;
  printSchemaArray(os, schema, indent);
  return os.str();
}

std::string print(const SchemaObject& schema, int indent) {
  std::ostringstream os;
  printSchemaObject(os, schema, indent);
  return os.str();
}

RAMPSON_END