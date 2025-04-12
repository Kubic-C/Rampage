# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Rampage")



# if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#     message("USING ASAN FOR DEBUG BUILD ON THE MSVC PLATFORM")
# 	add_compile_options("/fsanitize=address") 
#     add_compile_definitions("_DISABLE_VECTOR_ANNOTATION")
#     add_link_options("/INCREMENTAL:NO")
# endif()

# Used empty generator expression for some visual studio magic bullshitery
# See here ->>> https://stackoverflow.com/a/71721733

set(CMAKE_PDB_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin")


# Include sub-projects.
add_subdirectory("deps")
add_subdirectory("src")


add_custom_target(copy_freetype ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$ENV{FREETYPE_WINDOWS_BINARIES_PATH}/release dll/win64/freetype.dll" 
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Copying freetype.dll to the bin directory"
)

add_custom_target(copy_box2d ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:box2d>/box2d.dll"
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Copying box2d.dll to the bin directory"
)

# Ensure the copy_dlls target runs after building the application
add_dependencies(Rampage copy_freetype)