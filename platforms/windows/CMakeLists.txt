# Windows Platform CMake Configuration
# Build configuration for Windows platform implementation

cmake_minimum_required(VERSION 3.16)
project(FoundryEngineWindows VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific compiler flags
if(MSVC)
    add_compile_options(/W4 /MP)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0A00)
else()
    add_compile_options(-Wall -Wextra -Wno-unknown-pragmas)
endif()

# Find required packages
find_package(OpenGL REQUIRED)

# Source files
set(WINDOWS_SOURCES
    WindowsPlatform.cpp
    graphics.cpp
    audio.cpp
    input.cpp
    network.cpp
    system.cpp
    animation.cpp
)

set(WINDOWS_HEADERS
    WindowsPlatform.h
    WindowsPlatformPAL.h
)

# Create Windows platform library
add_library(foundry_windows_platform STATIC
    ${WINDOWS_SOURCES}
    ${WINDOWS_HEADERS}
)

# Include directories
target_include_directories(foundry_windows_platform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(foundry_windows_platform
    PUBLIC
        d3d11
        dxgi
        d3dcompiler
        xaudio2
        xinput
        shlwapi
        psapi
        pdh
        ws2_32
        comdlg32
        ole32
        oleaut32
        uuid
        gdi32
        user32
        kernel32
        advapi32
        shell32
    PRIVATE
        ${OPENGL_LIBRARIES}
)

# Platform-specific definitions
target_compile_definitions(foundry_windows_platform
    PRIVATE
        FOUNDRY_WINDOWS_PLATFORM
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

# Set output properties
set_target_properties(foundry_windows_platform PROPERTIES
    OUTPUT_NAME "foundry_windows"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation
install(TARGETS foundry_windows_platform
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${WINDOWS_HEADERS}
    DESTINATION include/FoundryEngine/platforms/windows
)

# Subdirectories (if any)
# add_subdirectory(examples)
# add_subdirectory(tests)

# Print configuration summary
message(STATUS "Windows Platform Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  OpenGL Found: ${OPENGL_FOUND}")
