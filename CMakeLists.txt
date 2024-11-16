cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(UM_BUILD_PLUGS)

project (Umugu VERSION 0.9.0)

add_subdirectory(umugu)
if (UM_BUILD_MAKER)
    add_subdirectory(umugu-maker)
    set_target_properties(umugu-maker PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
endif()

if (UM_BUILD_PLUGS)
    add_subdirectory(umugu-plugs)
endif()

set_target_properties(player-portaudio PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(player-stdout PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
