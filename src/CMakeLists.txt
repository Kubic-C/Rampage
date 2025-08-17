add_subdirectory("common")
add_subdirectory("host")
add_subdirectory("core")
add_subdirectory("log")

add_custom_target("RBuildAll" DEPENDS RCommon RHost RCore RLog)