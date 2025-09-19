add_library(RLog STATIC "log.hpp" "log.cpp" "module.hpp" "module.cpp")
set_property(TARGET RLog PROPERTY CXX_STANDARD 20)

target_link_libraries(RLog PUBLIC RCommon)