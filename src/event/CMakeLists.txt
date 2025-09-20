add_library(REvent STATIC
  "module.hpp" "module.cpp"
  "keys.hpp")
set_property(TARGET REvent PROPERTY CXX_STANDARD 20)

target_link_libraries(REvent PUBLIC RCommon)