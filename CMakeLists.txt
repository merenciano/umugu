cmake_minimum_required(VERSION 3.13) #TODO(Check the real minimum)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project (Umugu VERSION 0.3.0)

add_subdirectory(umugu)
add_subdirectory(umugu-maker)
add_subdirectory(umg-plugs)

set_target_properties(umg-player PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(umg-pipe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(umugu-maker PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
