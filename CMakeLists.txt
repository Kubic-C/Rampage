# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.12)

cmake_policy(SET CMP0076 NEW)

if (MSVC)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
endif ()

project ("Rampage")

set(ASAN_ON TRUE)
if(ASAN_ON)
  if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("USING ASAN FOR DEBUG BUILD ON THE MSVC PLATFORM")
    add_compile_options("/fsanitize=address")
    add_compile_definitions("_DISABLE_VECTOR_ANNOTATION")
    add_link_options("/INCREMENTAL:NO")
  endif()
endif()

# Used empty generator expression for some visual studio magic bullshitery
# See here ->>> https://stackoverflow.com/a/71721733

set(CMAKE_PDB_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin/debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<0:>${CMAKE_SOURCE_DIR}/bin")

# Include sub-projects.
add_subdirectory("deps")
add_subdirectory("src")

if(ASAN_ON)
  if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("USING ASAN FOR DEBUG BUILD ON THE MSVC PLATFORM")
    add_compile_options("/Zi")
  endif()
endif()

# Ensure the copy_dlls target runs after building the application
set_target_properties(box2d PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/debug"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/debug"
    PDB_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/debug"
)