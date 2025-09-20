add_library(RGame STATIC "module.hpp" "module.cpp")
set_property(TARGET RGame PROPERTY CXX_STANDARD 20)

target_link_libraries(RGame PUBLIC RCommon)