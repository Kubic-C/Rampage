#pragma once

#include "rampson.hpp"
#include <sstream>
#include <fstream>
#include <iostream>

using nlohmann::json;
using namespace rmp::json;

std::string compile(const json& schema, const std::string& sourceFile = "", std::string namespaceName = "JSchema") {
  auto schemaType = SchemaParser::parse(schema);
  return SchemaCompiler::compile(*schemaType, sourceFile, namespaceName);
}