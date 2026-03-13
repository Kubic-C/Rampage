add_library(RGui STATIC "module.hpp" "module.cpp")
set_property(TARGET RGui PROPERTY CXX_STANDARD 20)

target_sources(RGui PUBLIC
  "gui/common.hpp"
  "gui/gui.hpp"
  "gui/gui.cpp"
)

target_link_libraries(RGui PUBLIC RCommon RRender)