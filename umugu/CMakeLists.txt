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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/umugu
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_sources(umugu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/umugu/umugu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/umugu/umugu_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/umugu_nodes.c
	${CMAKE_CURRENT_SOURCE_DIR}/extern/src/dlfcn.c
)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

add_link_options(
    "LINKER:-rpath,${CMAKE_CURRENT_SOURCE_DIR}/../assets/plugs"
    #-rdynamic
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/extern/include
)

link_libraries(
	m
    #dl
	pthread
	#rt
    umugu
)

add_executable(plumugu)

target_sources(plumugu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test/main.c
	${CMAKE_CURRENT_SOURCE_DIR}/extern/src/dlfcn.c
)

target_link_directories(plumugu PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/extern/lib
)

target_link_libraries(plumugu PRIVATE
	Ole32
    fluidsynth-3
)
