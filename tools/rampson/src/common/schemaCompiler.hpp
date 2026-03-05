#pragma once

#include "schema.hpp"
#include <string>
#include <vector>
#include <ostream>

RAMPSON_START

// Compiles a parsed SchemaType tree into a generated C++ header containing
// struct definitions with getters, setters, and static metadata methods.
//
// Usage:
//   auto schemaType = SchemaParser::parse(jsonDoc);
//   std::string hpp = SchemaCompiler::compile(*schemaType, "mySchema.json");
//
// The generated output mirrors a cap'n-proto-style interface:
//   - Each JSON Schema object becomes a C++ struct
//   - Required properties get direct getters/setters
//   - Optional properties wrap in std::optional with has*() checks
//   - Static metadata getters are emitted only when present in the schema
//   - A nested Properties struct exposes per-property metadata & constraints
class SchemaCompiler {
public:
  static std::string compile(const SchemaType& schema, const std::string& sourceFile = "", std::string namespaceName = "JSchema");

private:
  struct ClassInfo {
    std::string name;
    const SchemaObject* obj;
    const SchemaType* schemaType;
  };

  static std::string toPascalCase(const std::string& input);

  // Walk the schema tree bottom-up, collecting every object type.
  static void collectTypes(const SchemaType& schema, const std::string& contextName,
                           std::vector<ClassInfo>& out);

  // Map a SchemaType to its C++ type string.
  static std::string cppTypeFor(const SchemaType& schema, const std::string& contextName);

  // Convenience: resolve the C++ type for a single ObjectProperty.
  static std::string propCppType(const ObjectProperty& prop);

  // Emit a full struct definition for one ClassInfo.
  static void emitClass(std::ostream& os, const ClassInfo& info);

  // Emit type-specific constraint metadata (min, max, pattern, etc.).
  static void emitTypeConstraints(std::ostream& os, const SchemaType& schema,
                                  const std::string& indent);

  // Emit the static validate() method for one ClassInfo.
  static void emitValidate(std::ostream& os, const ClassInfo& info);

  // Emit the static fromJson() method for one ClassInfo.
  static void emitFromJson(std::ostream& os, const ClassInfo& info);

  // Emit validation code for a single property within validate().
  static void emitPropertyValidation(std::ostream& os, const ObjectProperty& prop,
                                     const std::string& indent);

  // Emit a discriminated union (oneOf with discriminator) struct definition.
  static void emitOneOfClass(std::ostream& os, const ClassInfo& info);
};

RAMPSON_END
