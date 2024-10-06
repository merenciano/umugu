cmake_minimum_required(VERSION 3.13) #TODO(Check the real minimum)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project (Umugu VERSION 0.0.1)

set(UMUGU_BACKEND_PORTAUDIO true)

set(UMUGU_BACKEND_STDOUT false)

add_compile_definitions(
    $<$<BOOL:${UMUGU_BACKEND_PORTAUDIO}>:UMUGU_BACKEND_PORTAUDIO>
    $<$<BOOL:${UMUGU_BACKEND_STDOUT}>:UMUGU_BACKEND_STDOUT>
)

add_subdirectory(umugu)
add_subdirectory(umg-editor)
add_subdirectory(umg-plugs)

set_target_properties(umg-player PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set_target_properties(umg-editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
