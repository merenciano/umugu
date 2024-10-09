cmake_minimum_required(VERSION 3.13)

project(UmuguPlugs VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(Inspector SHARED)

set_target_properties(Inspector PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs)
set_target_properties(Inspector PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs)
set_target_properties(Inspector PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs)

target_compile_options(Inspector PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
    -fno-rtti
    -fno-exceptions
)

target_include_directories(Inspector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../umugu/include
)

target_sources(Inspector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inspector.cpp
)
