# Game Engine

A high-performance, cross-platform game engine written in C++ with Go networking support.

## Features

### Core Systems
- **ECS Architecture**: Entity-Component-System with efficient component management
- **Math Library**: 3D vectors, matrices, quaternions with optimized operations
- **Memory Management**: Object pooling and spatial hashing for performance optimization
- **Animation System**: Skeletal animation with keyframe interpolation and inverse kinematics
- **AI Systems**: Flocking behavior and A* pathfinding algorithms

### Networking
- **Go Network Server**: UDP-based multiplayer server implementation
- **Client Support**: Network client for connecting to servers
- **Peer-to-Peer Ready**: Foundation for direct peer connections

### Rendering
- **Cross-Platform Graphics**: OpenGL/Metal/WebGL implementations
- **VR Support**: Virtual reality rendering capabilities
- **Advanced Rendering**: Deferred shading and PBR material support

### Platform Support
- **Desktop**: Windows, Linux, macOS
- **Mobile**: Android, iOS
- **Web**: Emscripten/WebAssembly

## Build Instructions

### Prerequisites
- CMake 3.16+
- C++20 compatible compiler
- Go (for networking server)
- Platform-specific dependencies (OpenGL, etc.)

### Building

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Networking Server

To run the Go networking server:
```bash
cd src/engine/network/server
go run server.go
```

### Client Example

To run the Go network client:
```bash
cd src/engine/network/client
go run client.go
```

## Architecture

- `src/engine/core/`: Main C++ engine components
- `src/engine/network/`: Go networking implementation
- `src/engine/platforms/`: Platform-specific code
- CMake-based build system with platform-specific configurations

## License

[Add license information here]
