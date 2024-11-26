set(CMAKE_DEBUG_POSTFIX _d)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_library(umugu STATIC)

set_target_properties(umugu PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_compile_options(umugu PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
)

target_compile_definitions(umugu PUBLIC
    UMUGU_VERBOSE
    UMUGU_DEBUG
    UMUGU_TRACE
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/umugu/umugu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu_serialization.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/oscilloscope.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/wav_player.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/mixer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/limiter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/amplitude.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/nodes/output.c
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

add_executable(plumugu)

target_sources(plumugu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test/main.c
)

target_link_libraries(plumugu PRIVATE
    jack
    asound
    portaudio
    fluidsynth
)
