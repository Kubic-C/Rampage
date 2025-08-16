
# The Hosting Application for all hot swappin'

add_executable(RHost "main.cpp")
set_property(TARGET RHost PROPERTY CXX_STANDARD 20)

target_link_libraries(RHost PUBLIC RCommon)