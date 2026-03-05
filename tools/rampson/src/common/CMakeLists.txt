
add_library(Rampson STATIC 
  "commondef.hpp"
  "commondef.cpp"
  "schema.hpp"
  "schema.cpp"
  "schemaParser.hpp"
  "schemaParser.cpp"
  "schemaCompiler.hpp"
  "schemaCompiler.cpp")
  
set_property(TARGET Rampson PROPERTY CXX_STANDARD 20)
target_compile_features(Rampson PUBLIC cxx_std_20)

target_link_libraries(Rampson
  nlohmann_json::nlohmann_json)

target_include_directories(Rampson PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include") 

