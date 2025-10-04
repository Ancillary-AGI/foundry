# Windows Platform Implementation

This directory contains the Windows-specific implementation of the Foundry Game Engine platform abstraction layer (PAL).

## Overview

The Windows platform implementation provides native Windows support using:
- **DirectX 11** for graphics rendering
- **XAudio2** for audio processing
- **XInput** for gamepad input
- **WinSock** for networking
- **Windows API** for system services

## Directory Structure

```
platforms/windows/
├── WindowsPlatform.h/.cpp      # Main platform implementation
├── WindowsPlatformPAL.h        # Platform abstraction layer
├── graphics.cpp                # DirectX 11 graphics system
├── audio.cpp                   # XAudio2 audio system
├── input.cpp                   # Windows input management
├── network.cpp                 # WinSock networking
├── system.cpp                  # File system, timer, random
├── animation.cpp               # Animation system
├── CMakeLists.txt              # Build configuration
└── README.md                   # This file
```

## Components

### Graphics (DirectX 11)
- **WindowsGraphics**: Main graphics class handling DirectX 11 device/swap chain
- **WindowsD3DContext**: WebGL-compatible graphics context
- Features: Vertex/pixel shaders, buffers, textures, framebuffers

### Audio (XAudio2)
- **WindowsAudio**: Audio system with mastering voice management
- **WindowsAudioContext**: Audio context for source voices and processing
- Features: 3D audio, effects processing, mastering

### Input Management
- **WindowsInput**: Unified input handling for keyboard, mouse, gamepad
- Features: XInput gamepad support, raw input, event handling

### System Services
- **WindowsFileSystem**: Windows-specific file operations and paths
- **WindowsTimer**: High-resolution timing using QueryPerformanceCounter
- **WindowsRandom**: Random number generation with proper seeding

### Networking
- **WindowsNetworking**: WinSock-based networking implementation
- Features: TCP/UDP sockets, HTTP client, WebSocket support

### Animation
- **WindowsAnimation**: Skeletal animation system
- Features: Keyframe interpolation, bone transformations, clip management

## Building

### Prerequisites
- Windows 10 or later
- Visual Studio 2019 or later (or MinGW-w64)
- Windows SDK 10.0.18362.0 or later
- CMake 3.16 or later

### Build Instructions

1. **Using CMake (Recommended)**
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

2. **Using Visual Studio**
   - Open the project in Visual Studio
   - Set configuration to Release
   - Build the solution

3. **Using MinGW**
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=mingw.cmake
   make
   ```

## Platform Capabilities

### Graphics
- DirectX 11.1/11.0/10.1/10.0 support
- Hardware-accelerated rendering
- Multi-threading support
- Shader Model 5.0/4.0 support

### Audio
- XAudio2 with HRTF support
- Low-latency audio processing
- 3D spatial audio
- Audio effects and mastering

### Input
- XInput gamepad support (up to 4 controllers)
- Raw keyboard and mouse input
- Touch input support (via Windows Pointer API)
- Force feedback support

### System
- Windows shell integration
- Registry access for settings
- Performance monitoring
- Memory management

## Integration

The Windows platform integrates with the main engine through the PlatformInterface:

```cpp
// Initialize Windows platform
auto platform = std::make_unique<WindowsPlatform>(hInstance);

// Create window
platform->createWindow(1920, 1080, "My Game");

// Get graphics context
auto graphics = platform->getGraphics();
auto context = graphics->createContext();

// Main loop
while (platform->isRunning()) {
    platform->processMessages();

    // Update game logic
    update(deltaTime);

    // Render frame
    context->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render();
    graphics->present();
}
```

## Performance Considerations

- Use appropriate feature levels for target hardware
- Enable debug layer only in development builds
- Optimize shader compilation and caching
- Use proper resource management and cleanup

## Platform-Specific Features

### Windows Integration
- Native window creation and management
- Windows taskbar integration
- High DPI awareness and scaling
- Windows notification system

### DirectX Features
- Feature level detection and fallback
- Multi-GPU support (SLI/CrossFire)
- DirectX 12 Ultimate support (future)
- Hardware-accelerated video decoding

## Troubleshooting

### Common Issues

1. **DirectX Not Available**
   - Ensure Windows 10 with Platform Update
   - Install latest graphics drivers
   - Check DirectX runtime version

2. **Audio Issues**
   - Verify XAudio2 redistributables
   - Check audio device configuration
   - Ensure proper audio format support

3. **Input Problems**
   - Verify XInput controller compatibility
   - Check Windows Pointer API support
   - Ensure proper focus management

### Debug Configuration

For debugging, ensure these preprocessor definitions:
```cpp
#define _DEBUG
#define D3D11_CREATE_DEVICE_DEBUG
```

## Contributing

When adding new features:
1. Follow the existing code structure
2. Use RAII patterns for resource management
3. Add proper error handling and logging
4. Update CMakeLists.txt for new dependencies
5. Test on multiple Windows versions

## License

This Windows platform implementation follows the same license as the main Foundry Game Engine project.
