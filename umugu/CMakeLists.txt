cmake_minimum_required(VERSION 3.13)

set(CMAKE_DEBUG_POSTFIX _d)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

project (UmuguLib VERSION 0.2.0)

add_library(umugu STATIC)

set_target_properties(umugu PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set_target_properties(umugu PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_compile_options(umugu PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
)

target_include_directories(umugu PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/umugu
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(umugu PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(umugu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_playback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/builtin_nodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/oscilloscope.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/waveform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/wav_player.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/limiter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/amplitude.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/portaudio_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/stdout_impl.h
)

add_executable(umg-player)
target_compile_options(umg-player PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_options(umg-player PRIVATE
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs"
    -rdynamic
)

target_include_directories(umg-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_sources(umg-player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

target_link_libraries(umg-player PRIVATE
	m
    dl
	pthread
	rt
    umugu
)

if (${UMUGU_BACKEND_PORTAUDIO})
    target_link_libraries(umg-player PRIVATE
        jack
        asound
        portaudio
    )
endif()
