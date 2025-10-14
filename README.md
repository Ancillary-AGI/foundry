# FoundryEngine

A high-performance, cross-platform game engine written in C++ with a modern IDE built in Kotlin Multiplatform.

## Features

### Core Systems
- **ECS Architecture**: Entity-Component-System with efficient component management and SIMD optimizations
- **Math Library**: 3D vectors, matrices, quaternions with optimized operations
- **Memory Management**: Advanced object pooling and spatial hashing for performance optimization
- **Animation System**: Skeletal animation with keyframe interpolation and inverse kinematics
- **AI Systems**: Flocking behavior, A* pathfinding, and behavior trees

### Rendering
- **Cross-Platform Graphics**: OpenGL, DirectX 11, and Vulkan implementations
- **Advanced Rendering**: Deferred shading, PBR materials, and post-processing effects
- **VR Support**: Virtual reality rendering capabilities
- **WebAssembly**: Full engine support for web deployment

### Networking
- **Go Network Server**: Ultra-fast UDP-based multiplayer server implementation
- **Client Support**: Network client with client-side prediction and reconciliation
- **Peer-to-Peer Ready**: Foundation for direct peer connections

### Platform Support
- **Desktop**: Windows, Linux, macOS
- **Mobile**: Android, iOS
- **Web**: Emscripten/WebAssembly
- **Consoles**: PlayStation, Xbox (planned)

### IDE Features
- **Cross-Platform IDE**: Built with Kotlin Multiplatform for JVM, JS, and Native
- **Visual Editor**: Drag-and-drop entity and component management
- **Code Editor**: Syntax highlighting, IntelliSense, and debugging support
- **Asset Browser**: Integrated asset management and preview
- **Build System**: Multi-target building with CMake integration
- **Debugging**: Real-time debugging and profiling tools

## Quick Start

### Prerequisites
- CMake 3.16+
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Go 1.19+ (for networking server)
- Platform-specific dependencies (OpenGL, etc.)

### Building the Engine

```bash
# Clone the repository
git clone https://github.com/yourusername/foundry-engine.git
cd foundry-engine

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run example
./FoundryExample
```

### Building for Web

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build for WebAssembly
cd ../foundry-engine
mkdir build_web
cd build_web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make
```

### Building the IDE

```bash
# Install Kotlin Multiplatform
# Follow instructions at https://kotlinlang.org/docs/multiplatform.html

# Build IDE
cd ide
./gradlew build

# Run IDE
./gradlew run
```

### Running the Networking Server

```bash
cd networking/server
go run server.go
```

### Running the Network Client

```bash
cd networking/client
go run client.go
```

## Architecture

### Engine Core
- `src/Engine.cpp` - Main engine class and initialization
- `src/World.cpp` - ECS world management
- `src/Scene.cpp` - Scene management and hierarchy
- `src/MemoryPool.cpp` - Advanced memory management

### Systems
- `src/systems/` - Core engine systems (Audio, Input, Physics, etc.)
- `src/graphics/` - Rendering implementations
- `src/networking/` - Network client/server implementations

### IDE
- `ide/src/commonMain/` - Shared IDE logic
- `ide/src/jvmMain/` - JVM-specific implementations
- `ide/src/jsMain/` - JavaScript/Web implementations

### Platform Support
- `platforms/` - Platform-specific implementations
- `include/GameEngine/platform/` - Platform abstraction layer

## Project Structure

```
foundry-engine/
├── src/                    # Engine source code
│   ├── Engine.cpp         # Main engine implementation
│   ├── World.cpp          # ECS world
│   ├── Scene.cpp          # Scene management
│   ├── systems/           # Engine systems
│   ├── graphics/          # Rendering implementations
│   └── networking/        # Network implementations
├── include/               # Header files
│   └── GameEngine/        # Engine headers
├── ide/                   # IDE source code
│   ├── src/commonMain/    # Shared IDE logic
│   ├── src/jvmMain/       # JVM implementations
│   └── src/jsMain/        # JS implementations
├── platforms/             # Platform-specific code
├── networking/            # Go networking server/client
├── tests/                 # Test suite
├── examples/              # Example applications
├── CMakeLists.txt         # Build configuration
└── README.md             # This file
```

## Usage Examples

### Basic Engine Usage

```cpp
#include "GameEngine/core/Engine.h"

int main() {
    Engine& engine = Engine::getInstance();
    
    if (!engine.initialize()) {
        return -1;
    }
    
    // Create entities and components
    auto world = engine.getWorld();
    auto entityId = world->createEntity();
    world->addComponent<TransformComponent>(entityId, Vector3(0, 0, 0));
    
    // Run main loop
    while (engine.isRunning()) {
        engine.update(engine.getDeltaTime());
        engine.render();
    }
    
    engine.shutdown();
    return 0;
}
```

### Creating a Project with the IDE

```kotlin
val ideApp = IdeApplication()
val project = ProjectInfo(
    name = "MyGame",
    path = "/path/to/project",
    entities = emptyList(),
    components = getDefaultComponents(),
    systems = getDefaultSystems()
)

if (ideApp.createProject(project.name, project.path)) {
    println("Project created successfully!")
}
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Development Guidelines

- Follow the existing code style and conventions
- Write tests for new features
- Update documentation as needed
- Ensure cross-platform compatibility
- Use meaningful commit messages

## Testing

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test
./tests/TestMemoryPool
```

## Performance

FoundryEngine is designed for high performance:

- **SIMD Optimizations**: Vector operations use SIMD instructions
- **Memory Pooling**: Efficient memory management with object pooling
- **Spatial Partitioning**: Optimized collision detection and rendering
- **Multi-threading**: Job system for parallel processing
- **Cache-friendly**: Data-oriented design for better cache performance

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- OpenGL, DirectX, and Vulkan communities
- Emscripten team for WebAssembly support
- Kotlin Multiplatform team
- Go networking community
- All contributors and testers

## Roadmap

### Version 1.1
- [ ] WebGPU support
- [ ] Enhanced VR support
- [ ] Advanced AI systems
- [ ] Procedural generation tools

### Version 1.2
- [ ] Console platform support
- [ ] Advanced networking features
- [ ] Cloud integration
- [ ] Performance profiling tools

### Version 2.0
- [ ] Visual scripting system
- [ ] Advanced material editor
- [ ] Real-time collaboration
- [ ] Plugin system

## Support

- Documentation: [docs.foundryengine.dev](https://docs.foundryengine.dev)
- Issues: [GitHub Issues](https://github.com/yourusername/foundry-engine/issues)
- Discussions: [GitHub Discussions](https://github.com/yourusername/foundry-engine/discussions)
- Discord: [FoundryEngine Discord](https://discord.gg/foundryengine)

---

**FoundryEngine** - Building the future of game development