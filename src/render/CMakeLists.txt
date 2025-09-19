add_library(RRender STATIC ""module.hpp" "module.cpp")
set_property(TARGET RRender PROPERTY CXX_STANDARD 20)

target_link_libraries(RRender PUBLIC RCommon)