cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project (UmuguMaker VERSION 0.2.0)

find_library(IMGUI_FOUND imgui ${CMAKE_CURRENT_SOURCE_DIR}/extern/lib)

if (${IMGUI_FOUND} STREQUAL "IMGUI_FOUND-NOTFOUND")
    add_library(imgui STATIC)

    set_target_properties(imgui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/lib)
    set_target_properties(imgui PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern/lib)

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

endif()

add_executable(umugu-maker)

target_compile_options(umugu-maker PUBLIC
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(umugu-maker PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../umugu/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(umugu-maker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UmuguMaker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UmuguMaker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineInspector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineInspector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities.h
)

target_link_options(umugu-maker PRIVATE 
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs"
    -rdynamic
)

target_link_directories(umugu-maker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../umugu/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/lib
)

target_link_libraries(umugu-maker PRIVATE
	m
	pthread
	rt
    umugu
    GL
    dl
    SDL2
    -l:libimgui.a
    portaudio
    jack
)
