cmake_minimum_required(VERSION 3.16)
project(FoundryEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
endif()

# Build configuration
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_PROFILING "Enable profiling support" ON)
option(ENABLE_NETWORKING "Enable networking support" ON)
option(ENABLE_SCRIPTING "Enable scripting support" ON)
option(ENABLE_PHYSICS "Enable physics support" ON)
option(ENABLE_AUDIO "Enable audio support" ON)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG _DEBUG)
    if(MSVC)
        add_compile_options(/Od /Zi)
    else()
        add_compile_options(-O0 -g)
    endif()
else()
    add_compile_definitions(NDEBUG)
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# Find required packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Platform-specific dependencies
if(PLATFORM_WINDOWS)
    find_package(DirectX QUIET)
endif()

if(PLATFORM_LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(OPENAL REQUIRED openal)
endif()

# Third-party libraries
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# Bullet Physics (if enabled)
if(ENABLE_PHYSICS)
    find_package(Bullet QUIET)
    if(NOT Bullet_FOUND)
        message(STATUS "Bullet Physics not found, using built-in version")
        add_subdirectory(${THIRD_PARTY_DIR}/bullet3 EXCLUDE_FROM_ALL)
        set(BULLET_LIBRARIES BulletDynamics BulletCollision LinearMath)
    endif()
endif()

# Lua (if scripting enabled)
if(ENABLE_SCRIPTING)
    find_package(Lua QUIET)
    if(NOT LUA_FOUND)
        message(STATUS "Lua not found, using built-in version")
        add_subdirectory(${THIRD_PARTY_DIR}/lua EXCLUDE_FROM_ALL)
        set(LUA_LIBRARIES lua)
    endif()
endif()

# OpenAL (if audio enabled)
if(ENABLE_AUDIO AND NOT PLATFORM_WINDOWS)
    if(NOT OPENAL_FOUND)
        find_package(OpenAL REQUIRED)
    endif()
endif()

# Engine source files
set(ENGINE_SOURCES
    src/Engine.cpp
    src/main.cpp
    src/World.cpp
    src/Scene.cpp
    src/MemoryPool.cpp
    src/SerializationSystem.cpp
    src/networking/UDPNetworking.cpp
    src/systems/AssetSystem.cpp
    src/systems/AssetPipeline.cpp
    src/systems/AudioSystem.cpp
    src/systems/InputSystem.cpp
    src/systems/PhysicsSystem.cpp
    src/systems/ScriptingSystem.cpp
    src/systems/UISystem.cpp
    src/systems/NetworkSystem.cpp
    src/systems/ProfilerSystem.cpp
    src/systems/UpdateSystem.cpp
    src/graphics/OpenGLRenderer.cpp
    src/graphics/D3D11Renderer.cpp
    src/web/WebAssemblyBindings.cpp
    src/typescript/NativeTypeScriptRuntime.cpp
    src/typescript/TypeScriptToNativeCompiler.cpp
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    list(APPEND ENGINE_SOURCES
        platforms/windows/WindowsPlatform.cpp
        platforms/windows/graphics.cpp
        platforms/windows/audio.cpp
        platforms/windows/input.cpp
        platforms/windows/network.cpp
        platforms/windows/system.cpp
    )
elseif(PLATFORM_LINUX)
    list(APPEND ENGINE_SOURCES
        platforms/linux/LinuxPlatform.cpp
    )
elseif(PLATFORM_MACOS)
    list(APPEND ENGINE_SOURCES
        platforms/macos/macOSPlatform.swift
    )
endif()

# Android sources
if(ANDROID)
    list(APPEND ENGINE_SOURCES
        platforms/android/AndroidPlatform.cpp
        platforms/android/graphics.cpp
        platforms/android/input.cpp
        platforms/android/network.cpp
        platforms/android/system.cpp
        platforms/android/audio/AAudioPlatform.cpp
        platforms/android/core/PlatformServices.cpp
    )
endif()

# Create engine library
add_library(FoundryEngine ${ENGINE_SOURCES})

# Include directories
target_include_directories(FoundryEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms
)

# Link libraries
target_link_libraries(FoundryEngine PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${OPENGL_LIBRARIES}
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(FoundryEngine PUBLIC
        d3d11.lib
        dxgi.lib
        d3dcompiler.lib
        xaudio2.lib
        xinput.lib
        ws2_32.lib
        winmm.lib
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(FoundryEngine PUBLIC
        ${SDL2_LIBRARIES}
        ${OPENAL_LIBRARY}
        GL
        X11
        pthread
        dl
    )
elseif(PLATFORM_MACOS)
    target_link_libraries(FoundryEngine PUBLIC
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Metal"
        "-framework MetalKit"
        "-framework AVFoundation"
    )
endif()

# Optional dependencies
if(ENABLE_PHYSICS)
    target_link_libraries(FoundryEngine PUBLIC ${BULLET_LIBRARIES})
    target_compile_definitions(FoundryEngine PUBLIC ENABLE_PHYSICS)
endif()

if(ENABLE_SCRIPTING)
    target_link_libraries(FoundryEngine PUBLIC ${LUA_LIBRARIES})
    target_compile_definitions(FoundryEngine PUBLIC ENABLE_SCRIPTING)
endif()

if(ENABLE_NETWORKING)
    target_compile_definitions(FoundryEngine PUBLIC ENABLE_NETWORKING)
endif()

if(ENABLE_AUDIO)
    target_compile_definitions(FoundryEngine PUBLIC ENABLE_AUDIO)
endif()

if(ENABLE_PROFILING)
    target_compile_definitions(FoundryEngine PUBLIC ENABLE_PROFILING)
endif()

# Web/WASM build
if(EMSCRIPTEN)
    set_target_properties(FoundryEngine PROPERTIES
        COMPILE_FLAGS "-s USE_SDL=2 -s USE_WEBGL2=1"
        LINK_FLAGS "-s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1"
    )
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS FoundryEngine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FoundryEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FoundryEngineConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FoundryEngineConfig.cmake"
    INSTALL_DESTINATION lib/cmake/FoundryEngine
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FoundryEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FoundryEngineConfigVersion.cmake"
    DESTINATION lib/cmake/FoundryEngine
)

# CPack configuration
set(CPACK_PACKAGE_NAME "FoundryEngine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance cross-platform game engine")
set(CPACK_PACKAGE_VENDOR "FoundryEngine Team")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()
include(CPack)
