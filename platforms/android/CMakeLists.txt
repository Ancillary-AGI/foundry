# Android Platform CMake Configuration
cmake_minimum_required(VERSION 3.18.1)

project(FoundryEngineAndroid)

# Set Android-specific flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${ANDROID_NDK}/sources/android/native_app_glue
)

# Source files
set(ANDROID_PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidPlatform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AndroidFileSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AndroidTimer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AndroidNetworking.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AndroidRandom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/system.cpp
)

# Create Android platform library
add_library(foundry-android SHARED ${ANDROID_PLATFORM_SOURCES})

# Link libraries
target_link_libraries(foundry-android
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
    curl  # For HTTP networking
    vulkan  # For Vulkan support
)

# Compiler definitions
target_compile_definitions(foundry-android PRIVATE
    ANDROID_PLATFORM=1
    VULKAN_SUPPORT=1
    GLES3_SUPPORT=1
)

# Compiler flags
target_compile_options(foundry-android PRIVATE
    -Wall
    -Wextra
    -O3
    -ffast-math
    -fno-exceptions
    -fno-rtti
)

# Export symbols for JNI
set_target_properties(foundry-android PROPERTIES
    LINK_FLAGS "-Wl,--export-dynamic"
)

# Install target
install(TARGETS foundry-android
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)