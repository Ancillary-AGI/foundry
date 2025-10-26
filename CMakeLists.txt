cmake_minimum_required(VERSION 3.20)
project(FoundryEngine VERSION 2.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
option(FOUNDRY_BUILD_TESTS "Build tests" ON)
option(FOUNDRY_BUILD_EXAMPLES "Build examples" ON)
option(FOUNDRY_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(FOUNDRY_ENABLE_VULKAN "Enable Vulkan support" ON)
option(FOUNDRY_ENABLE_RAYTRACING "Enable ray tracing" ON)
option(FOUNDRY_ENABLE_VR "Enable VR support" ON)
option(FOUNDRY_ENABLE_NETWORKING "Enable networking" ON)
option(FOUNDRY_ENABLE_AI "Enable AI systems" ON)

# Platform detection
if(WIN32)
    set(FOUNDRY_PLATFORM "Windows")
    add_definitions(-DFOUNDRY_PLATFORM_WINDOWS)
elseif(APPLE)
    set(FOUNDRY_PLATFORM "macOS")
    add_definitions(-DFOUNDRY_PLATFORM_MACOS)
elseif(UNIX)
    set(FOUNDRY_PLATFORM "Linux")
    add_definitions(-DFOUNDRY_PLATFORM_LINUX)
endif()

# Compiler-specific settings
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    add_compile_options(/arch:AVX2) # Enable SIMD
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Werror -pedantic)
    add_compile_options(-mavx2 -mfma) # Enable SIMD
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DFOUNDRY_DEBUG)
    add_compile_options(-g -O0)
else()
    add_definitions(-DFOUNDRY_RELEASE)
    add_compile_options(-O3 -DNDEBUG)
endif()

# Find required packages
find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include
    ${Vulkan_INCLUDE_DIRS}
)

# Core engine library
set(FOUNDRY_CORE_SOURCES
    # Core systems
    src/GameEngine/core/Engine.cpp
    src/GameEngine/core/System.cpp
    src/GameEngine/core/ECS.cpp
    src/GameEngine/core/MemoryManager.cpp
    
    # Math library
    src/GameEngine/math/Vector2.cpp
    src/GameEngine/math/Vector3.cpp
    src/GameEngine/math/Vector4.cpp
    src/GameEngine/math/Matrix4.cpp
    src/GameEngine/math/Quaternion.cpp
    
    # Graphics systems
    src/GameEngine/graphics/RenderPipeline.cpp
    src/GameEngine/graphics/VulkanRenderer.cpp
    src/GameEngine/graphics/RayTracingSystem.cpp
    
    # Physics systems
    src/GameEngine/physics/PhysicsSystem.cpp
    src/GameEngine/physics/FluidSimulation.cpp
    src/GameEngine/physics/ClothPhysics.cpp
    
    # Audio systems
    src/GameEngine/audio/AudioSystem.cpp
    src/GameEngine/audio/SpatialAudio.cpp
    src/GameEngine/audio/ProceduralAudio.cpp
    
    # AI systems
    src/GameEngine/ai/AISystem.cpp
    src/GameEngine/ai/NeuralNetwork.cpp
    src/GameEngine/ai/BehaviorTree.cpp
    
    # Networking
    src/GameEngine/networking/NetworkSystem.cpp
    src/GameEngine/networking/NetworkPrediction.cpp
    src/GameEngine/networking/AntiCheat.cpp
    
    # VR/AR systems
    src/GameEngine/vr/VRSystem.cpp
    src/GameEngine/vr/SpatialMapping.cpp
    src/GameEngine/vr/VRInteraction.cpp
    
    # Cloud integration
    src/GameEngine/cloud/CloudSystem.cpp
    src/GameEngine/cloud/AssetStreaming.cpp
    src/GameEngine/cloud/MultiplayerBackend.cpp
    
    # Security
    src/GameEngine/security/SecuritySystem.cpp
    src/GameEngine/security/CodeObfuscator.cpp
    src/GameEngine/security/AntiCheatEngine.cpp
    src/GameEngine/security/DRMSystem.cpp
    
    # Mobile systems
    src/GameEngine/mobile/MobileSystem.cpp
    src/GameEngine/mobile/AdaptiveRenderer.cpp
    src/GameEngine/mobile/MobileUI.cpp
    
    # Character systems
    src/GameEngine/character/CharacterSystem.cpp
    src/GameEngine/character/ProceduralGeneration.cpp
    src/GameEngine/character/AnimationSystem.cpp
    
    # TypeScript runtime
    src/GameEngine/typescript/TypeScriptRuntime.cpp
    src/GameEngine/typescript/TypeScriptJITCompiler.cpp
    src/GameEngine/typescript/TypeScriptModule.cpp
    
    # Platform abstraction
    src/GameEngine/platform/PlatformInterface.cpp
    src/GameEngine/platform/WindowSystem.cpp
    src/GameEngine/platform/InputSystem.cpp
)

# Create core library
add_library(FoundryEngine STATIC ${FOUNDRY_CORE_SOURCES})

# Link libraries
target_link_libraries(FoundryEngine
    ${Vulkan_LIBRARIES}
    ${OPENGL_LIBRARIES}
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(FoundryEngine
        d3d11 d3d12 dxgi
        xaudio2 xinput
        ws2_32 winmm
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL_LIBRARY Metal)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    target_link_libraries(FoundryEngine
        ${COCOA_LIBRARY}
        ${METAL_LIBRARY}
        ${COREAUDIO_LIBRARY}
    )
elseif(UNIX)
    find_package(X11 REQUIRED)
    find_package(ALSA REQUIRED)
    target_link_libraries(FoundryEngine
        ${X11_LIBRARIES}
        ${ALSA_LIBRARIES}
        GL EGL
    )
endif()

# TypeScript runtime library
add_library(FoundryTypeScript SHARED
    src/GameEngine/typescript/TypeScriptRuntime.cpp
    src/GameEngine/typescript/TypeScriptJITCompiler.cpp
    src/GameEngine/typescript/TypeScriptModule.cpp
    src/GameEngine/typescript/TypeScriptValue.cpp
)

target_link_libraries(FoundryTypeScript FoundryEngine)

# IDE integration library
add_library(FoundryIDE SHARED
    ide/src/commonMain/kotlin/com/foundry/ide/core/IDECore.kt
    ide/src/commonMain/kotlin/com/foundry/ide/agentic/AgenticDevelopmentEnvironment.kt
    ide/src/commonMain/kotlin/com/foundry/ide/mcp/MCPClient.kt
)

# Examples
if(FOUNDRY_BUILD_EXAMPLES)
    # Basic TypeScript game
    add_executable(TypeScriptGameExample
        examples/typescript-game/src/main.cpp
    )
    target_link_libraries(TypeScriptGameExample FoundryEngine FoundryTypeScript)
    
    # Native TypeScript game
    add_executable(NativeTypeScriptGameExample
        examples/native-typescript-game/src/main.cpp
    )
    target_link_libraries(NativeTypeScriptGameExample FoundryEngine FoundryTypeScript)
    
    # Complete demo
    add_executable(CompleteGameDemo
        examples/complete-game-demo/src/main.cpp
    )
    target_link_libraries(CompleteGameDemo FoundryEngine FoundryTypeScript)
    
    # VR demo
    if(FOUNDRY_ENABLE_VR)
        add_executable(VRDemo
            examples/vr-demo/src/main.cpp
        )
        target_link_libraries(VRDemo FoundryEngine FoundryTypeScript)
    endif()
endif()

# Tests
if(FOUNDRY_BUILD_TESTS)
    enable_testing()
    
    add_executable(FoundryEngineTests
        tests/AdvancedEngineTests.cpp
        tests/TypeScriptRuntimeTests.cpp
        tests/PhysicsTests.cpp
        tests/NetworkingTests.cpp
        tests/AITests.cpp
        tests/SecurityTests.cpp
    )
    
    target_link_libraries(FoundryEngineTests FoundryEngine FoundryTypeScript)
    
    add_test(NAME EngineTests COMMAND FoundryEngineTests)
endif()

# Installation
install(TARGETS FoundryEngine FoundryTypeScript
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Package configuration
set(CPACK_PACKAGE_NAME "FoundryEngine")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "Advanced Game Engine with TypeScript-First Development")
set(CPACK_PACKAGE_CONTACT "team@foundryengine.com")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;RPM")
endif()

include(CPack)

# Print configuration summary
message(STATUS "=== Foundry Engine Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Platform: ${FOUNDRY_PLATFORM}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SIMD Enabled: ${FOUNDRY_ENABLE_SIMD}")
message(STATUS "Vulkan Enabled: ${FOUNDRY_ENABLE_VULKAN}")
message(STATUS "Ray Tracing Enabled: ${FOUNDRY_ENABLE_RAYTRACING}")
message(STATUS "VR Enabled: ${FOUNDRY_ENABLE_VR}")
message(STATUS "Networking Enabled: ${FOUNDRY_ENABLE_NETWORKING}")
message(STATUS "AI Enabled: ${FOUNDRY_ENABLE_AI}")
message(STATUS "Build Tests: ${FOUNDRY_BUILD_TESTS}")
message(STATUS "Build Examples: ${FOUNDRY_BUILD_EXAMPLES}")
message(STATUS "=====================================")