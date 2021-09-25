cmake_minimum_required(VERSION 3.15)

project(sml)

include(cmake/StandardProjectSettings.cmake)

add_library(sml_project_options INTERFACE)
add_library(sml_project_warnings INTERFACE)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(sml_project_warnings)

include(cmake/Sanitizers.cmake)
enable_sanitizers(sml_project_options)

add_subdirectory(libsml)
add_subdirectory(smlexample)