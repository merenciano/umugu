cmake_minimum_required(VERSION 3.13)

#set(CMAKE_DEBUG_POSTFIX _d)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project (UmuguLib VERSION 0.0.1)

add_library(umugu STATIC)

set_target_properties(umugu PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set_target_properties(umugu PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_compile_options(umugu PUBLIC
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
    #-g3
    #-fsanitize=address,undefined
)

target_include_directories(umugu PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/umugu
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(umugu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(umugu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu_portaudio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/builtin_nodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/oscilloscope.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/waveform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/wav_player.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/limiter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/amplitude.c
)

add_executable(umg-player)
target_compile_options(umg-player PUBLIC 
    -Wall
    -Wextra
    -Wpedantic
    #-g3
    #-fsanitize=address,undefined
)

target_link_options(umg-player PUBLIC
    #-fsanitize=address,undefined
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs"
    -rdynamic
)

target_include_directories(umg-player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(umg-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

target_link_libraries(umg-player PRIVATE
	m
    dl
	pthread
	portaudio
	rt
	asound
	jack
    umugu
)
