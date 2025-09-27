# FoundryEngine Web Build

This directory contains the web build configuration and scripts for the FoundryEngine game engine.

## 🚀 Quick Start

### Prerequisites

1. **Install Emscripten SDK**
   ```bash
   # On Ubuntu/Debian
   sudo apt-get install emscripten

   # On macOS with Homebrew
   brew install emscripten

   # On Windows with MSYS2
   pacman -S mingw-w64-x86_64-emscripten
   ```

2. **Verify Installation**
   ```bash
   emcc --version
   ```

### Building the Engine

1. **Build for Web**
   ```bash
   ./build_web.sh
   ```

2. **Build for WASM**
   ```bash
   ./build_wasm.sh
   ```

## 📁 Output Files

After building, you'll find these files in the respective build directories:

- `foundryengine.js` - JavaScript module for web
- `foundryengine.wasm` - WebAssembly binary for web
- `foundryengine_wasm.js` - JavaScript module for WASM
- `foundryengine_wasm.wasm` - WebAssembly binary for WASM

## 🌐 Usage

### Basic Web Integration

```html
<!DOCTYPE html>
<html>
<head>
    <title>FoundryEngine Game</title>
    <script src='build/web/foundryengine.js'></script>
</head>
<body>
    <canvas id="canvas" width="800" height="600"></canvas>
    <script>
        const engine = FoundryEngine();
        engine.onRuntimeInitialized = () => {
            console.log('FoundryEngine loaded!');
            // Initialize your game here
        };
    </script>
</body>
</html>
```

### Advanced Usage with Custom Canvas

```javascript
const engine = FoundryEngine({
    canvas: document.getElementById('gameCanvas'),
    noInitialRun: true
});

engine.onRuntimeInitialized = () => {
    // Engine is ready
    engine.callMain(['--width', '1920', '--height', '1080']);
};
```

## ⚙️ Configuration

### Memory Settings

- **Initial Memory**: 128MB
- **Maximum Memory**: 512MB
- **Stack Size**: 512KB

### Graphics Settings

- **WebGL2**: Enabled
- **Offscreen Canvas**: Supported
- **Audio Worklet**: Enabled

### Build Flags

The build scripts use optimized Emscripten flags for:
- Maximum performance (`-O3`)
- WebAssembly output
- ES6 module support
- Memory growth support
- WebGL2 graphics

## 🔧 Development

### Custom Build

You can modify the build scripts to customize:

1. **Memory allocation**:
   ```bash
   -s INITIAL_MEMORY=268435456  # 256MB
   ```

2. **Graphics features**:
   ```bash
   -s USE_WEBGPU=1              # Enable WebGPU
   ```

3. **Debugging**:
   ```bash
   -s ASSERTIONS=1              # Enable assertions
   -s SAFE_HEAP=1               # Enable heap safety
   ```

### Platform-Specific Builds

- **Web**: Optimized for browser deployment
- **WASM**: Optimized for WebAssembly environments

## 🐛 Troubleshooting

### Common Issues

1. **Emscripten not found**
   - Ensure Emscripten is installed and in your PATH
   - Source the Emscripten environment: `source /path/to/emsdk/emsdk_env.sh`

2. **Build fails with missing files**
   - Ensure all source files exist in the correct locations
   - Check that the web platform files are present

3. **Runtime errors**
   - Check browser console for WebAssembly errors
   - Verify WebGL2 support in the target browser

### Performance Tips

1. **Enable hardware acceleration** in your browser
2. **Use appropriate memory settings** for your target devices
3. **Consider using WebAssembly threads** for CPU-intensive tasks
4. **Optimize assets** for web deployment

## 📚 Resources

- [Emscripten Documentation](https://emscripten.org/docs/)
- [WebAssembly](https://webassembly.org/)
- [WebGL2 Specification](https://www.khronos.org/registry/webgl/specs/latest/2.0/)

## 🤝 Contributing

When contributing to the web build:

1. Test on multiple browsers (Chrome, Firefox, Safari, Edge)
2. Verify mobile compatibility
3. Check WebAssembly compatibility
4. Update this documentation for any new features

---

**FoundryEngine** - The World's Most Advanced Cross-Platform Game Engine
