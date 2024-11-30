cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(UM_BUILD_PLUGS "Build external plugins" on)
option(UM_BUILD_MAKER "Build graphical tool for umugu" off)
option(UM_BUILD_TRACY "Enable trace profiling with tracy" off)

project (Umugu VERSION 0.9.0)

add_subdirectory(umugu)

if (UM_BUILD_TRACY)
    include(FetchContent)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(tracy)
    target_link_libraries(umugu PUBLIC TracyClient)
endif()

if (UM_BUILD_MAKER)
    add_subdirectory(umugu-maker)
    set_target_properties(umugu-maker PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
endif()

if (UM_BUILD_PLUGS)
    add_subdirectory(umugu-plugs)
endif()

set_target_properties(plumugu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
