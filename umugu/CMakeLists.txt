set(CMAKE_DEBUG_POSTFIX _d)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/builtin_nodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/oscilloscope.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/waveform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/wav_player.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/limiter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/amplitude.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/tube_amp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/peakeq_filter.c
)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

add_link_options(
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs"
    -rdynamic
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_libraries(
	m
    dl
	pthread
	rt
    umugu
)

add_executable(player-portaudio)

target_sources(player-portaudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test/umugutest_portaudio.c
)

target_link_libraries(player-portaudio PRIVATE
    jack
    asound
    portaudio
)

add_executable(player-stdout)

target_sources(player-stdout PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test/umugutest_stdout.c
)
