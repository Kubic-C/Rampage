
# The Core App

add_library(RCore SHARED "entry.cpp"
  ../common/empty.cpp)
set_property(TARGET RCore PROPERTY CXX_STANDARD 20)

target_link_libraries(RCore PUBLIC RCommon)