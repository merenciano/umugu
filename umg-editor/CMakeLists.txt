cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project (Edmugu VERSION 0.0.1)

add_library(imgui STATIC)

target_include_directories(imgui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src
)
target_sources(imgui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_impl_sdl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/implot_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/implot_items.cpp
)

add_executable(umg-editor)

target_compile_options(umg-editor PUBLIC
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(umg-editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../umugu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(umg-editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu_editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu_editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_impl_sdl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/implot_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/src/implot_items.cpp
)

target_link_options(umg-editor PUBLIC
    #-fsanitize=address,undefined
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs"
    -rdynamic
)

target_link_directories(umg-editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../umugu/lib
)

target_link_libraries(umg-editor PRIVATE
	m
	pthread
	portaudio
	rt
	asound
	jack
    umugu
    GL
    dl
    SDL2
)