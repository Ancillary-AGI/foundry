# FoundryEngine Web Platform

A comprehensive WebAssembly/WebGL implementation of the FoundryEngine Platform Abstraction Layer (PAL) with UDP networking support.

## 🚀 Overview

The Web platform enables FoundryEngine to run in web browsers through WebAssembly and WebGL, providing the same powerful features available on native platforms including:

- **WebGL 2.0 Graphics** with full shader support
- **Web Audio API** for spatial and low-latency audio
- **WebRTC Networking** for peer-to-peer connections
- **UDP Networking** for multiplayer games
- **IndexedDB Storage** for persistent data
- **Service Workers** for offline functionality
- **Touch & Gamepad Input** for mobile gaming

## 🏗️ Architecture

```
Web Platform PAL
├── WebPlatformPAL (Main PAL implementation)
│   ├── WebGraphicsContext (WebGL 2.0)
│   ├── WebAudioContext (Web Audio API)
│   ├── WebInputContext (Touch, Keyboard, Gamepad)
│   ├── WebNetworkContext (UDP + WebRTC)
│   ├── WebStorageContext (IndexedDB)
│   ├── WebPlatformServices (Cloud Save, Push Notifications)
│   ├── WebWindowManager (Canvas management)
│   └── WebEventSystem (DOM event handling)
```

## 📋 Requirements

### Development Environment
- **Emscripten SDK** 3.1.0 or later
- **Node.js** 16+ (for build tools)
- **Python** 3.8+ (Emscripten dependency)
- **CMake** 3.16+ with Emscripten toolchain

### Browser Support
- **Chrome/Edge** 88+ (WebGL 2.0, WebRTC, IndexedDB)
- **Firefox** 85+ (WebGL 2.0, WebRTC, IndexedDB)
- **Safari** 14+ (WebGL 2.0, WebRTC, IndexedDB)
- **Mobile Safari** iOS 14.5+
- **Chrome Android** 88+

## 🛠️ Building

### 1. Install Emscripten

```bash
# Clone Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest SDK
./emsdk install latest
./emsdk activate latest

# Set up environment (add to your shell profile)
source ./emsdk_env.sh
```

### 2. Configure Build

```bash
# Create build directory
mkdir build_web && cd build_web

# Configure with Emscripten
emcmake cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -s USE_WEBGL2=1 -s USE_SDL=2" \
    -DCMAKE_EXE_LINKER_FLAGS="-s ALLOW_MEMORY_GROWTH=1 -s WASM=1"

# Build
make -j$(nproc)
```

### 3. Alternative: Use Build Script

```bash
# Use the provided build script
./build_web.sh
```

## 🚀 Running

### Local Development Server

```bash
# Install a local web server (if not using build script)
npm install -g http-server

# Serve the built files
http-server build_web/ -p 8080 -c-1 --cors
```

### Access the Application

Open your browser to: `http://localhost:8080/GameEngine.html`

## ⚙️ Configuration

### Emscripten Compilation Flags

```cmake
# Essential flags for Web platform
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
    -O3                                    # Optimization level
    -s USE_WEBGL2=1                       # Enable WebGL 2.0
    -s USE_SDL=2                          # SDL2 support
    -s FULL_ES3=1                         # OpenGL ES 3.0
    -s ALLOW_MEMORY_GROWTH=1              # Dynamic memory growth
    -s WASM=1                             # WebAssembly output
    -s ASYNCIFY=1                         # Async operations
    -s FETCH=1                            # HTTP requests
    -s WEBSOCKET_URL='ws://localhost:8080' # WebSocket support
")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}
    -s EXPORTED_FUNCTIONS=['_main','_createEngine','_runEngine','_destroyEngine']
    -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']
    -s ENVIRONMENT='web'
    -s MINIFY_HTML=0                      # Keep HTML readable
")
```

### Web-Specific Configuration

```cpp
// Configure Web platform in your application
PlatformConfig config;
config.windowWidth = 1280;
config.windowHeight = 720;
config.fullscreen = false;
config.vsync = true;

// Web-specific settings
config.web.canvasId = "gameCanvas";
config.web.powerPreference = "high-performance";
config.web.antialias = true;
config.web.premultipliedAlpha = true;
```

## 🌐 Networking

### UDP Networking (Primary)

The Web platform includes full UDP networking support for multiplayer games:

```cpp
#include "GameEngine/networking/UDPNetworking.h"

// Create UDP networking instance
auto networking = FoundryEngine::createUDPNetworking();
networking->initialize();

// Connect to Go server
auto connection = networking->createConnection();
connection->connect("your-server.com", 8080);

// Send game data
FoundryEngine::UDPPacket packet;
packet.type = FoundryEngine::UDPPacketType::PlayerInput;
connection->sendPacket(packet, true); // Reliable
```

### WebRTC (Peer-to-Peer)

For direct peer-to-peer connections:

```cpp
// Create WebRTC peer connection
webNetworkContext->createPeerConnection();
webNetworkContext->createDataChannel("gameData");

// Handle ICE candidates and signaling
webNetworkContext->addIceCandidate(candidateString);
```

## 🎮 Input Handling

### Touch Input

```cpp
// Touch events are automatically handled
webInputContext->getTouchPosition(touchId, x, y);
int touchCount = webInputContext->getTouchCount();
```

### Gamepad Support

```cpp
// Gamepad API is fully supported
int gamepadCount = webInputContext->getGamepadCount();
// Standard gamepad mapping
```

### Pointer Lock

```cpp
// Enable FPS-style mouse look
webPlatform->enablePointerLock();
bool locked = webPlatform->isPointerLocked();
```

## 💾 Storage

### IndexedDB (Persistent Storage)

```cpp
// Automatic cloud save functionality
webPlatformServices->saveToCloud("playerData", data);
webPlatformServices->loadFromCloud("playerData", data);
```

### Local Storage

```cpp
// Fast key-value storage
webStorageContext->writeFile("settings.json", settingsData);
webStorageContext->readFile("settings.json", settingsData);
```

## 🔊 Audio

### Web Audio API

```cpp
// Spatial audio with HRTF
webAudioContext->createAudioWorklet("spatialAudio", workletCode);
webAudioContext->loadAudioBuffer("sound.wav", "mySound");
webAudioContext->playBuffer("mySound", false);
```

### Low-Latency Audio

```cpp
// ScriptProcessorNode for real-time audio
webAudioContext->initializeLowLatencyAudio();
```

## 📱 Mobile Support

### Progressive Web App (PWA)

The Web platform includes full PWA support:

```json
// manifest.json
{
  "name": "FoundryEngine Game",
  "short_name": "FoundryGame",
  "start_url": "/",
  "display": "fullscreen",
  "background_color": "#000000",
  "theme_color": "#000000",
  "icons": [...]
}
```

### Touch Controls

```html
<!-- Virtual joystick for mobile -->
<div class="joystick-container">
  <div class="joystick" id="virtual-joystick">
    <div class="joystick-knob"></div>
  </div>
</div>
```

### Orientation Handling

```cpp
// Automatic orientation changes
webPlatform->setOrientation(orientation);
```

## 🐛 Debugging

### Browser DevTools

```javascript
// Access WebAssembly memory
const memory = Module.HEAPU8;
console.log('Memory size:', memory.length);

// Call exported functions
Module.ccall('debugFunction', 'void', [], []);

// Monitor performance
console.log('FPS:', Module.fps);
```

### Debug Panel

The Web platform includes a built-in debug panel showing:
- FPS and frame time
- Memory usage
- WebGL information
- Network statistics
- Platform details

Press `F` to toggle the debug panel.

## 📊 Performance Optimization

### WebAssembly Optimization

```cmake
# Aggressive optimization for web
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -flto")
```

### Memory Management

```cpp
// WebAssembly has limited memory by default
// Use ALLOW_MEMORY_GROWTH for dynamic allocation
-s ALLOW_MEMORY_GROWTH=1

// Monitor memory usage
size_t used = webPlatform->getUsedMemory();
size_t total = webPlatform->getTotalMemory();
```

### Asset Loading

```cpp
// Use Emscripten's fetch API for assets
EM_ASM({
    fetch('assets/texture.png')
        .then(response => response.arrayBuffer())
        .then(buffer => {
            // Process asset data
        });
});
```

## 🔧 Advanced Features

### Service Workers

```javascript
// Offline functionality
webPlatformServices->registerForPushNotifications();
webPlatformServices->scheduleNotification("Game Update", "New content available!", 3600);
```

### Web Workers

```cpp
// Background processing
webPlatform->createWebWorker("physicsWorker.js");
webPlatform->postMessageToWorker(workerId, data);
```

### WebRTC Data Channels

```cpp
// Real-time multiplayer
webNetworkContext->createDataChannel("gameState");
webNetworkContext->sendDataChannelMessage("gameState", gameData);
```

## 🧪 Testing

### Automated Testing

```bash
# Run Web platform tests
npm test

# Browser compatibility testing
npx playwright test
```

### Manual Testing

```bash
# Start development server with hot reload
npm run dev

# Test on different browsers
npm run test:chrome
npm run test:firefox
npm run test:safari
```

## 📚 Examples

### Basic Web Application

```cpp
#include "platforms/web/WebPlatformPAL.h"

int main() {
    // Create Web platform
    auto platform = std::make_unique<FoundryEngine::WebPlatformPAL>();
    platform->initialize();

    // Create your game engine
    auto engine = createGameEngine(platform.get());

    // Main loop (handled by Emscripten)
    return 0;
}
```

### Multiplayer Game

See `examples/web_multiplayer_example.html` for a complete multiplayer implementation.

## 🚀 Deployment

### Web Servers

```bash
# Nginx configuration
server {
    listen 80;
    server_name yourgame.com;

    location / {
        root /path/to/built/game;
        try_files $uri $uri/ /GameEngine.html;
    }

    # Enable gzip compression
    gzip on;
    gzip_types text/plain text/css application/json application/javascript application/wasm;
}
```

### CDN Deployment

```bash
# Upload to CDN
aws s3 sync build_web/ s3://your-cdn-bucket/ --delete

# Invalidate CloudFront
aws cloudfront create-invalidation --distribution-id YOUR_DIST_ID --paths "/*"
```

## 🔮 Future Enhancements

- **WebGPU Support** (when browser support matures)
- **WebTransport** for improved networking
- **WebCodecs** for video/audio processing
- **SharedArrayBuffer** for multi-threading
- **WebAssembly SIMD** for performance
- **WebAssembly GC** for better memory management

## 📖 API Reference

### WebPlatformPAL

```cpp
class WebPlatformPAL : public PlatformInterface {
public:
    // Core methods
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // Web-specific methods
    void setCanvasId(const std::string& canvasId);
    void enablePointerLock();
    bool isPointerLocked() const;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE getWebGLContext() const;
};
```

### WebNetworkContext

```cpp
class WebNetworkContext : public NetworkContext {
public:
    bool connect(const std::string& host, int port) override;
    int send(const void* data, size_t size) override;
    int receive(void* buffer, size_t size) override;

    // WebRTC methods
    void createPeerConnection();
    void createDataChannel(const std::string& name);
    void sendDataChannelMessage(const std::string& channel, const std::string& message);
};
```

## 🐛 Troubleshooting

### Common Issues

**WebGL Context Lost**
```
Error: WebGL context lost
Solution: Handle context restoration in your application
```

**CORS Issues**
```
Access to XMLHttpRequest blocked by CORS policy
Solution: Configure server CORS headers or use same origin
```

**Memory Growth Issues**
```
Out of memory error
Solution: Monitor memory usage and optimize allocations
```

**WebRTC Connection Failed**
```
ICE connection failed
Solution: Check STUN/TURN server configuration
```

## 📄 License

This Web platform implementation is part of FoundryEngine and follows the same license terms.

---

The Web platform brings FoundryEngine's full power to the browser, enabling high-performance games with multiplayer networking, advanced graphics, and native-like experiences across all devices! 🌐🎮⚡
