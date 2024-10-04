cmake_minimum_required(VERSION 3.13)

project(umg-plugs VERSION 0.0.1)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/../umg-editor/extern/include
)

target_sources(Inspector PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inspector.cpp
)

target_link_libraries(Inspector PRIVATE
    imgui
)