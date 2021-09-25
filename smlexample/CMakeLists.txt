cmake_minimum_required(VERSION 3.15)

add_executable(smlexample smlexample.cpp)

target_link_libraries(smlexample PRIVATE sml_project_options sml_project_warnings sml)