# Web Platform CMake Configuration for Emscripten
cmake_minimum_required(VERSION 3.16)

project(FoundryEngineWeb)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emscripten-specific settings
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    # Include directories
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    )
    
    # Source files
    set(WEB_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/WebPlatformPAL.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/GameEngine/core/EnhancedECS.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/GameEngine/graphics/AdvancedRenderPipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/GameEngine/physics/AdvancedPhysicsSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/GameEngine/audio/AdvancedAudioSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/GameEngine/networking/AdvancedNetworkSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src/GameEngine/ai/AISystem.cpp
    )
    
    # Create Web platform executable
    add_executable(foundry-web ${WEB_PLATFORM_SOURCES})
    
    # Emscripten-specific compile flags
    target_compile_options(foundry-web PRIVATE
        -O3
        -ffast-math
        -DWEB_PLATFORM=1
        -DWEBGL_SUPPORT=1
        -DWEBRTC_SUPPORT=1
    )
    
    # Emscripten-specific link flags
    set_target_properties(foundry-web PROPERTIES
        LINK_FLAGS "
            -s USE_WEBGL2=1
            -s USE_GLFW=3
            -s WASM=1
            -s ALLOW_MEMORY_GROWTH=1
            -s INITIAL_MEMORY=67108864
            -s MAXIMUM_MEMORY=2147483648
            -s USE_PTHREADS=1
            -s PTHREAD_POOL_SIZE=4
            -s OFFSCREENCANVAS_SUPPORT=1
            -s FETCH=1
            -s WEBSOCKET_URL=ws://
            -s WEBSOCKET_SUBPROTOCOL=binary
            -s EXPORTED_FUNCTIONS=['_main','_malloc','_free']
            -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']
            -s MODULARIZE=1
            -s EXPORT_NAME='FoundryEngine'
            --preload-file assets
            --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/shell.html
        "
    )
    
    # Copy assets and shell file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/shell.html
        ${CMAKE_BINARY_DIR}/shell.html
        COPYONLY
    )
    
else()
    message(FATAL_ERROR "Web platform requires Emscripten toolchain")
endif()

# Install target
install(FILES 
    ${CMAKE_BINARY_DIR}/foundry-web.html
    ${CMAKE_BINARY_DIR}/foundry-web.js
    ${CMAKE_BINARY_DIR}/foundry-web.wasm
    ${CMAKE_BINARY_DIR}/foundry-web.data
    DESTINATION web
)