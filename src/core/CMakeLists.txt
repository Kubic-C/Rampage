add_library(RCore STATIC "module.hpp" "module.cpp")
set_property(TARGET RCore PROPERTY CXX_STANDARD 20)

target_link_libraries(RCore PUBLIC RCommon)