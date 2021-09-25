cmake_minimum_required(VERSION 3.15)

add_library(sml)
target_compile_features(sml PUBLIC cxx_std_14)

target_sources(sml 
PRIVATE
    include/sml.hpp
    source/sml.cpp    
)

target_include_directories(sml PUBLIC include)

target_link_libraries(sml PRIVATE sml_project_options sml_project_warnings)