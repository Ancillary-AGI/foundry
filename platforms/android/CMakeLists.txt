# Android CMake Configuration for FoundryEngine
cmake_minimum_required(VERSION 3.18)

# Set Android ABI
set(ANDROID_ABI armeabi-v7a arm64-v8a x86 x86_64)

# Set Android platform level (API 21 = Android 5.0)
set(ANDROID_PLATFORM android-21)

# Set Android STL
set(ANDROID_STL c++_static)

# Project configuration
project(FoundryEngineAndroid)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Android NDK
find_package(AndroidNdk REQUIRED)

# Include directories
include_directories(
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${ANDROID_NDK}/sources/android/cpufeatures
    ../../include
    ../../src
)

# Source files
set(SOURCES
    AndroidPlatform.cpp
    core/AndroidPlatformPAL.h
    core/PlatformServices.cpp
    core/PlatformServices.h
    audio/AAudioPlatform.cpp
    audio/AAudioPlatform.h
    graphics/VulkanPlatform.cpp
    graphics/VulkanPlatform.h
    graphics/GPUDebugger.cpp
    graphics/GPUDebugger.h
    input/GestureManager.cpp
    input/GestureManager.h
    input/StylusManager.cpp
    input/StylusManager.h
    network/EnhancedNetworking.cpp
    network/EnhancedNetworking.h
    system/AccessibilityManager.cpp
    system/AccessibilityManager.h
    system/PushNotificationManager.cpp
    system/PushNotificationManager.h
    system/ScopedStorageManager.cpp
    system/ScopedStorageManager.h
    system/ThermalManager.cpp
    system/ThermalManager.h
    system/BackgroundTaskManager.h
    ../../src/main.cpp
    ../../src/core/*.cpp
    ../../src/math/*.cpp
    ../../src/graphics/*.cpp
    ../../src/physics/*.cpp
    ../../src/ai/*.cpp
    ../../src/animation/*.cpp
    ../../src/networking/*.cpp
    ../../src/systems/*.cpp
    ../../src/world/*.cpp
    ../../src/components/*.cpp
    ../../src/optimization/*.cpp
)

# Create shared library
add_library(foundryengine SHARED ${SOURCES})

# Link libraries
target_link_libraries(foundryengine
    android
    log
    EGL
    GLESv3
    OpenSLES
    z
    m
    c
)

# Set target properties
set_target_properties(foundryengine PROPERTIES
    OUTPUT_NAME "foundryengine"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Android-specific compile options
target_compile_options(foundryengine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-missing-field-initializers
    -ffast-math
    -funroll-loops
    -fomit-frame-pointer
    -march=armv7-a
    -mtune=cortex-a8
    -mfloat-abi=softfp
    -mfpu=vfpv3-d16
    -DANDROID
    -D__ANDROID__
    -D__ARM_ARCH_7__
    -D__ARM_ARCH_7A__
)

# ARM64 specific options
target_compile_options(foundryengine PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)

# Set Android NDK properties
android_ndk_import_module_cpufeatures()
android_ndk_import_module_native_app_glue()

# Create APK target
set(APK_NAME "FoundryEngine.apk")
set(APK_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${APK_NAME}")

# Package APK (this would be done by Gradle in practice)
# For now, just create the native library
