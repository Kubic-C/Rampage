add_library(RLog SHARED "entry.cpp" "log.hpp" "log.cpp")
set_property(TARGET RLog PROPERTY CXX_STANDARD 20)

target_link_libraries(RLog PUBLIC RCommon)