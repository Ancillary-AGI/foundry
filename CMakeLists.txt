cmake_minimum_required(VERSION 3.16)
project(GameEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Add the executable
add_executable(GameEngine
    src/main.cpp
    src/networking/UDPNetworking.cpp
)

# Web-specific sources (Emscripten)
if(EMSCRIPTEN)
    target_sources(GameEngine PRIVATE
        platforms/web/WebPlatformPAL.cpp
        src/web/library.js
        src/web/loading.js
    )
endif()

# Configure for cross-platform
if(EMSCRIPTEN)
    # WebAssembly build (Emscripten)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s WASM=1 -s USE_WEBGL2=1 -s USE_SDL=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s MINIFY_HTML=0")
    add_definitions(-DEMSCRIPTEN)
elseif(APPLE)
    # macOS/iOS
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework Metal -framework Foundation")
    if(IOS)
        # iOS specific settings
        set(CMAKE_SYSTEM_NAME iOS)
        set(CMAKE_OSX_SYSROOT iphoneos)
    endif()
elseif(WIN32)
    # Windows
    add_definitions(-D_WIN32)
else()
    # Linux/Android
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    if(ANDROID)
        set(CMAKE_SYSTEM_NAME Android)
        set(CMAKE_SYSTEM_VERSION 21)
    endif()
endif()

# Add subdirectories
add_subdirectory(src)

# Platform-specific linking
if(EMSCRIPTEN)
    # WebAssembly linking
    set_target_properties(GameEngine PROPERTIES
        SUFFIX ".html"
        LINK_FLAGS "-s EXPORTED_FUNCTIONS=['_main','_createEngine','_runEngine','_destroyEngine']"
    )
elseif(APPLE)
    if(IOS)
        # iOS framework
        set_target_properties(GameEngine PROPERTIES
            MACOSX_BUNDLE YES
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.company.GameEngine"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        )
    else()
        # macOS application
        target_link_libraries(GameEngine PRIVATE "-framework Cocoa" "-framework MetalKit")
    endif()
elseif(WIN32)
    # Windows executable
    target_link_libraries(GameEngine PRIVATE opengl32.lib)
else()
    # Linux/Android
    target_link_libraries(GameEngine PRIVATE GL GLEW pthread)
endif()

# Installation
install(TARGETS GameEngine LIBRARY DESTINATION lib)
install(DIRECTORY include/GameEngine
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "Building for platform: ${CMAKE_SYSTEM_NAME}")
