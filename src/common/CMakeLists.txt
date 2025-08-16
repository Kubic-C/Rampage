add_library(RCommon STATIC
  "host.hpp"
  "module.hpp"
  "empty.cpp")
set_property(TARGET RCommon PROPERTY CXX_STANDARD 20)

target_link_libraries(RCommon PUBLIC cr glaze::glaze)