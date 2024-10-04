cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project (Umugu VERSION 0.0.1)

add_subdirectory(umugu)
add_subdirectory(umg-editor)
add_subdirectory(umg-plugs)

set_target_properties(umg-player PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(umg-editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
