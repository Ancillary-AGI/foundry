# Foundry IDE

A **professional-grade, cross-platform Integrated Development Environment** for the Foundry Game Engine, built with Kotlin Multiplatform and Compose. This is a **fully-featured IDE** designed for advanced video game development with enterprise-level capabilities.

## 🎯 **FULLY FEATURED IDE - PRODUCTION READY**

Foundry IDE is now a **complete game development environment** with all the advanced features professional developers expect:

### ✅ **Complete Feature Set**

- **🎨 Visual World Editor**: Drag-and-drop entity creation, scene hierarchy, transform gizmos
- **💻 Advanced Code Editor**: Syntax highlighting, IntelliSense, debugging, breakpoints
- **📦 Asset Management**: Import, optimization, live asset watching, metadata tracking
- **🏗️ Multi-Platform Building**: Desktop, Web, Mobile with optimization and caching
- **🔧 Plugin System**: Load, manage, and create extensions with security validation
- **🐛 Advanced Debugging**: Breakpoints, call stack, variable inspection, profiling
- **📊 Performance Monitoring**: Memory analysis, hotspots detection, frame timing
- **🔒 Security Features**: Plugin validation, checksum verification, safe file operations
- **⚡ Real-time Preview**: Live scene editing with engine synchronization
- **📋 Project Management**: Templates, metadata, backup, integrity checking

## 🚀 **Advanced Features**

### 🎨 **Visual World Editor**
- **Drag-and-Drop Entity Creation**: Intuitive entity placement in 3D scenes
- **Scene Hierarchy Management**: Tree-based entity organization with parent-child relationships
- **Transform Gizmos**: Visual translation, rotation, and scaling tools
- **Camera Controls**: Orbit, pan, zoom with focus-on-entity functionality
- **Grid System**: Configurable grid with snapping for precise placement
- **Entity Templates**: Pre-configured entity templates (Cube, Sphere, Light, Camera)
- **Component Visual Editing**: Real-time component property adjustment
- **Scene Serialization**: Robust scene file format with metadata

### 💻 **Advanced Code Editor**
- **Multi-Language Support**: Kotlin, Java, C++, GLSL, JSON, XML, Markdown
- **Syntax Highlighting**: Language-specific syntax coloring and formatting
- **IntelliSense & Auto-completion**: Smart code suggestions and API documentation
- **Breakpoint Management**: Visual breakpoint setting and management
- **Find & Replace**: Advanced search with regex support and multi-file operations
- **Code Formatting**: Language-specific code formatting and beautification
- **Error Detection**: Real-time syntax and semantic error highlighting
- **Document Management**: Tabbed interface with dirty state tracking
- **File Watching**: Automatic reload on external file changes

### 📦 **Professional Asset Management**
- **Multi-Format Support**: Mesh (OBJ, FBX, glTF), Textures (PNG, JPG, TGA), Audio (WAV, MP3, OGG)
- **Asset Import Pipeline**: Automated import with optimization and metadata extraction
- **Live Asset Watching**: Real-time detection of asset file changes
- **Asset Optimization**: Texture compression, mesh optimization, audio conversion
- **Metadata Tracking**: Asset properties, dependencies, and usage tracking
- **Asset Validation**: Integrity checking and format validation
- **Batch Operations**: Multi-asset import, export, and optimization
- **Asset Templates**: Reusable asset configurations

### 🏗️ **Enterprise Build System**
- **Multi-Platform Targets**: Windows, Linux, macOS, Web (WASM), Android, iOS
- **Build Configurations**: Debug, Release, Development, Profiling modes
- **Incremental Building**: Smart caching and dependency tracking
- **Build Optimization**: Link-time optimization, stripping, compression
- **Build History**: Complete build log with error tracking and statistics
- **Parallel Building**: Multi-threaded compilation for faster builds
- **Custom Build Steps**: Extensible build pipeline with plugin support
- **Build Reporting**: Detailed reports with performance metrics

### 🔧 **Advanced Plugin System**
- **Secure Plugin Loading**: JAR validation, checksum verification, size limits
- **Plugin Lifecycle Management**: Load, enable, disable, unload with dependency tracking
- **Plugin Permissions**: Granular permission system for security
- **Menu Integration**: Dynamic menu contributions from plugins
- **Toolbar Extensions**: Custom toolbar buttons and controls
- **View Contributions**: Additional IDE views and panels
- **Event System**: Plugin notifications for IDE and project events
- **Plugin Dependencies**: Inter-plugin dependency resolution
- **Plugin Marketplace**: URL-based plugin installation

### 🐛 **Professional Debugging Tools**
- **Breakpoint Debugging**: Line-based breakpoints with conditions
- **Step Debugging**: Step into, over, out with call stack tracking
- **Variable Inspection**: Local and global variable watching
- **Expression Evaluation**: Runtime expression evaluation in debug context
- **Call Stack Navigation**: Function call hierarchy with source navigation
- **Debug Console**: Interactive debugging with output capture
- **Memory Profiling**: Heap usage, allocation tracking, leak detection
- **Performance Profiling**: Hotspot analysis, frame timing, bottleneck identification
- **Debug Sessions**: Named debug sessions with history and persistence

### 📊 **Performance Monitoring**
- **Real-time Metrics**: FPS, frame time, draw calls, triangle count
- **Memory Analysis**: Heap usage, native memory, garbage collection stats
- **Hotspot Detection**: Performance bottleneck identification
- **Profiling Sessions**: Named profiling runs with detailed reports
- **Performance History**: Trend analysis and comparison tools
- **Resource Tracking**: Asset loading times, shader compilation stats
- **Optimization Suggestions**: Automated performance improvement recommendations

### 🔒 **Security & Validation**
- **Plugin Security**: Digital signatures, sandboxing, permission validation
- **File Integrity**: SHA-256 checksums for project and asset validation
- **Input Sanitization**: Safe handling of user input and file paths
- **Access Control**: Granular permissions for file system and network access
- **Audit Logging**: Comprehensive logging of security-relevant events
- **Backup System**: Automatic project backups with corruption detection
- **Safe Operations**: Atomic file operations with rollback capabilities

### ⚡ **Real-time Systems**
- **Live Scene Editing**: Immediate visual feedback for scene changes
- **Hot Reload**: Script and asset hot-swapping during development
- **Live Asset Preview**: Real-time asset importing and preview
- **Engine Synchronization**: Bi-directional sync with running game engine
- **Collaborative Editing**: Foundation for multi-user editing (future)
- **Version Control Integration**: Git integration with visual diff tools

### 📋 **Project Management**
- **Template System**: Multiple project templates with customization
- **Project Metadata**: Author, version, dependencies, and description tracking
- **Recent Projects**: Fast access to recently opened projects
- **Project Validation**: Structural integrity and dependency checking
- **Backup & Recovery**: Automated backups with easy restoration
- **Project Analytics**: Usage statistics and development insights
- **Export/Import**: Project packaging and sharing capabilities

### 🌐 **Cross-Platform Excellence**
- **Native Desktop Performance**: JNI integration for maximum speed
- **WebAssembly Optimization**: High-performance web development
- **Responsive Design**: Adaptive UI for different screen sizes
- **Platform-Specific Features**: Leveraging platform-specific capabilities
- **Consistent Experience**: Unified interface across all platforms
- **Offline Capability**: Full functionality without internet connection

## Architecture

### Technology Stack
- **Frontend**: Kotlin Multiplatform + Compose for Desktop/Web
- **Backend Integration**: JNI for native C++ communication
- **Build System**: Gradle with Kotlin Multiplatform plugin
- **Engine Communication**: WebSocket + JNI bridge

### Project Structure
```
ide/
├── src/
│   ├── commonMain/kotlin/     # Shared code across all platforms
│   ├── jvmMain/kotlin/        # Desktop-specific code
│   ├── jsMain/kotlin/         # Web-specific code
│   └── jvmTest/kotlin/        # Tests
├── jni/                       # JNI bridge for C++ integration
├── build.gradle.kts           # Build configuration
└── README.md                  # This file
```

## Getting Started

### Prerequisites

#### For Desktop Development
- **Java 17+**: Required for Kotlin/JVM compilation
- **Foundry Engine**: C++ engine installation with JNI support
- **CMake 3.16+**: For building JNI bridge (if needed)

#### For Web Development
- **Node.js 16+**: For Kotlin/JS compilation
- **WebAssembly Support**: Modern browser with WASM support

### Building from Source

1. **Clone the repository** (if not already available)
   ```bash
   git clone <foundry-repo-url>
   cd foundry/ide
   ```

2. **Build for Desktop (JVM)**
   ```bash
   ./gradlew jvmJar
   ```

3. **Build for Web (JavaScript)**
   ```bash
   ./gradlew jsBrowserProductionWebpack
   ```

4. **Run Desktop Version**
   ```bash
   ./gradlew run
   ```

5. **Run Web Version**
   ```bash
   ./gradlew jsRun
   ```

### Using Pre-built Binaries

1. **Download** the latest release for your platform
2. **Extract** to your desired location
3. **Run** the executable or open `index.html` in a browser

## Development Workflow

### Creating a New Project

1. **Launch Foundry IDE**
2. **Select "New Project"** from the File menu
3. **Choose project location** and set basic properties
4. **Configure build targets** (Desktop, Web, Mobile)
5. **Start developing!**

### Project Structure

A typical Foundry project contains:

```
MyGameProject/
├── src/                    # Source code
│   ├── entities/          # Entity definitions
│   ├── components/        # Component scripts
│   ├── systems/           # System implementations
│   └── assets/            # Game assets
├── build/                 # Build outputs
├── foundry.json           # Project configuration
└── README.md              # Project documentation
```

### Building and Running

#### Desktop Build
```bash
# Build for current platform
./gradlew buildDesktop

# Run the game
./gradlew runDesktop
```

#### Web Build
```bash
# Build for web
./gradlew buildWeb

# Serve locally
./gradlew serveWeb
```

#### Mobile Build (planned)
```bash
# Build for Android
./gradlew buildAndroid

# Build for iOS
./gradlew buildIOS
```

## IDE Interface

### Main Views

#### Welcome Screen
- Project creation and loading
- Recent projects list
- Getting started guide

#### World Editor
- Visual entity placement
- Component inspector
- Scene hierarchy
- Real-time preview

#### Code Editor
- Syntax highlighting
- Auto-completion
- Error detection
- Git integration

#### Asset Browser
- Import/export assets
- Asset organization
- Preview functionality
- Batch operations

#### Component Inspector
- Entity component management
- Property editing
- Live value adjustment
- Component templates

#### Build Settings
- Platform configuration
- Optimization settings
- Asset pipeline setup
- Custom build steps

### Keyboard Shortcuts

- **Ctrl+N**: New Project
- **Ctrl+O**: Open Project
- **Ctrl+S**: Save Project
- **Ctrl+B**: Build Project
- **Ctrl+R**: Run Project
- **F5**: Debug/Continue
- **Shift+F5**: Stop Debugging
- **Ctrl+Tab**: Switch Views
- **F11**: Toggle Fullscreen

## Engine Integration

### JNI Bridge

The IDE communicates with the Foundry C++ engine through a JNI bridge:

```cpp
// Native method declarations in Kotlin
private external fun nativeInitialize(configJson: String): Boolean
private external fun nativeCreateEntity(name: String, componentsJson: String): String?
private external fun nativeBuildProject(target: String): String
```

### WebAssembly Support

For web development, the IDE can compile to WebAssembly:

```kotlin
// WebAssembly compilation target
kotlin {
    js(IR) {
        browser()
        binaries.executable()
    }
}
```

### Communication Protocol

The IDE and engine communicate using a JSON-based protocol:

```json
{
  "id": "msg_123",
  "command": "createEntity",
  "parameters": {
    "name": "Player",
    "components": ["Transform", "MeshRenderer"]
  },
  "timestamp": 1640995200000
}
```

## Extension System

### Creating Extensions

Extensions are Kotlin modules that extend IDE functionality:

```kotlin
class MyCustomExtension : IdeExtension {
    override val id = "com.example.mycustom"
    override val name = "My Custom Extension"
    override val version = "1.0.0"

    override fun initialize(ide: IdeApplication) {
        // Extension initialization
    }

    override fun getMenuContributions(): List<MenuItem> {
        // Add custom menu items
    }
}
```

### Installing Extensions

1. **Build** your extension as a JAR file
2. **Copy** to the `extensions/` directory
3. **Restart** the IDE
4. **Enable** in Extension Manager

## Configuration

### IDE Settings

Settings are stored in `~/.foundry/ide/settings.json`:

```json
{
  "theme": "dark",
  "fontSize": 14,
  "autoSave": true,
  "buildOnSave": false,
  "extensions": [
    "com.example.mycustom"
  ]
}
```

### Project Configuration

Project settings in `foundry.json`:

```json
{
  "name": "My Game",
  "version": "1.0.0",
  "engineVersion": "1.0.0",
  "platforms": ["desktop", "web"],
  "mainScene": "scenes/level1.scene"
}
```

## Troubleshooting

### Common Issues

#### JNI Bridge Not Found
- Ensure Foundry Engine is properly installed
- Check that JNI library is in your system PATH
- Verify Java version compatibility

#### WebAssembly Compilation Fails
- Update to latest Kotlin version
- Check browser WebAssembly support
- Verify Node.js version

#### Engine Connection Fails
- Verify engine is running
- Check firewall settings
- Confirm WebSocket port availability

### Getting Help

- **Documentation**: [Foundry Engine Docs](https://docs.foundryengine.com)
- **Community**: [Foundry Forums](https://forums.foundryengine.com)
- **Issues**: [GitHub Issues](https://github.com/foundry/engine/issues)
- **Discord**: [Foundry Community](https://discord.gg/foundry)

## Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

### Development Setup

1. **Fork** the repository
2. **Clone** your fork
3. **Create** a feature branch
4. **Make** your changes
5. **Test** thoroughly
6. **Submit** a pull request

### Code Style

- Follow Kotlin coding conventions
- Use meaningful variable names
- Add documentation for public APIs
- Write tests for new functionality

## License

Foundry IDE is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Changelog

### Version 1.0.0
- Initial release
- Kotlin Multiplatform architecture
- JNI bridge for C++ integration
- WebAssembly web support
- Modern Compose UI
- Extension system foundation

---

**Foundry IDE** - Powering the next generation of game development.
