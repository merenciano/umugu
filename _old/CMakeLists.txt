cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(UM_BUILD_PORTAUDIO "Clone and build PortAudio" true)
option(UM_BUILD_PLUGS "Build external plugins" on)
option(UM_BUILD_MAKER "Build graphical tool for umugu" on)
option(UM_BUILD_TRACY "Enable trace profiling with tracy" off)

project (Umugu VERSION 0.9.0)

add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
)

add_subdirectory(umugu)

if (UM_BUILD_PORTAUDIO)
    include(FetchContent)
    #set(PA_BUILD_SHARED ON CACHE BOOL "Build shared PortAudio library" FORCE)
    set(PA_BUILD_STATIC ON CACHE BOOL "Build static PortAudio library" FORCE)

    if (CYGWIN)
        set(PA_USE_WASAPI ON CACHE BOOL "Enable WASAPI backend" FORCE)
        set(PA_USE_ASIO OFF CACHE BOOL "Enable ASIO backend" FORCE)
        set(PA_USE_COREAUDIO OFF CACHE BOOL "Enable CoreAudio backend" FORCE)
        set(PA_USE_ALSA OFF CACHE BOOL "Enable ALSA backend" FORCE)
        set(PA_USE_JACK OFF CACHE BOOL "Enable JACK backend" FORCE)
    elseif (MINGW)
        set(PA_USE_WASAPI ON CACHE BOOL "Enable WASAPI backend" FORCE)
        set(PA_USE_ASIO OFF CACHE BOOL "Enable ASIO backend" FORCE)
        set(PA_USE_COREAUDIO OFF CACHE BOOL "Enable CoreAudio backend" FORCE)
        set(PA_USE_ALSA OFF CACHE BOOL "Enable ALSA backend" FORCE)
        set(PA_USE_JACK OFF CACHE BOOL "Enable JACK backend" FORCE)
    elseif(WIN32)
        set(PA_USE_WASAPI ON CACHE BOOL "Enable WASAPI backend" FORCE)
        set(PA_USE_ASIO OFF CACHE BOOL "Enable ASIO backend" FORCE)
        set(PA_USE_COREAUDIO OFF CACHE BOOL "Enable CoreAudio backend" FORCE)
        set(PA_USE_ALSA OFF CACHE BOOL "Enable ALSA backend" FORCE)
        set(PA_USE_JACK OFF CACHE BOOL "Enable JACK backend" FORCE)
    elseif(APPLE)
        set(PA_USE_WASAPI OFF CACHE BOOL "Enable WASAPI backend" FORCE)
        set(PA_USE_ASIO OFF CACHE BOOL "Enable ASIO backend" FORCE)
        set(PA_USE_COREAUDIO ON CACHE BOOL "Enable CoreAudio backend" FORCE)
        set(PA_USE_ALSA OFF CACHE BOOL "Enable ALSA backend" FORCE)
        set(PA_USE_JACK OFF CACHE BOOL "Enable JACK backend" FORCE)
    elseif(UNIX)
        set(PA_USE_WASAPI OFF CACHE BOOL "Enable WASAPI backend" FORCE)
        set(PA_USE_ASIO OFF CACHE BOOL "Enable ASIO backend" FORCE)
        set(PA_USE_COREAUDIO OFF CACHE BOOL "Enable CoreAudio backend" FORCE)
        set(PA_USE_ALSA ON CACHE BOOL "Enable ALSA backend" FORCE)
        set(PA_USE_JACK OFF CACHE BOOL "Enable JACK backend" FORCE)
    endif()

    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG v19.7.0
    )

    FetchContent_MakeAvailable(portaudio)

    target_include_directories(umugu PRIVATE ${portaudio_SOURCE_DIR}/include)
    target_link_libraries(umugu PRIVATE portaudio)
    target_include_directories(plumugu PRIVATE ${portaudio_SOURCE_DIR}/include)
    target_link_libraries(plumugu PRIVATE portaudio)
endif()

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
    target_include_directories(umugu-maker PRIVATE ${portaudio_SOURCE_DIR}/include)
    target_link_libraries(umugu-maker PRIVATE portaudio)
    set_target_properties(umugu-maker PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
endif()

if (UM_BUILD_PLUGS)
    add_subdirectory(umugu-plugs)
endif()

set_target_properties(plumugu PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
