#!/bin/bash

# FoundryEngine WASM Build Script
# Builds the FoundryEngine for WebAssembly using Emscripten (optimized for WASM)

set -e

echo "🔨 Building FoundryEngine for WASM..."

# Check if Emscripten is installed
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found. Please install Emscripten SDK."
    echo "Visit: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create build directory
mkdir -p build/wasm
cd build/wasm

# Configure Emscripten for WASM
echo "⚙️ Configuring Emscripten WASM build..."

# Emscripten WASM configuration
EMSCRIPTEN_FLAGS=(
    "-O3"
    "-s WASM=1"
    "-s MODULARIZE=1"
    "-s EXPORT_NAME='FoundryEngineWASM'"
    "-s EXPORT_ES6=1"
    "-s USE_WEBGL2=1"
    "-s USE_WEBGPU=0"
    "-s FULL_ES3=1"
    "-s ALLOW_MEMORY_GROWTH=1"
    "-s INITIAL_MEMORY=134217728"
    "-s MAXIMUM_MEMORY=536870912"
    "-s STACK_SIZE=524288"
    "-s ASSERTIONS=0"
    "-s GL_ASSERTIONS=0"
    "-s INVOKE_RUN=0"
    "-s EXIT_RUNTIME=0"
    "-s NO_FILESYSTEM=0"
    "-s FORCE_FILESYSTEM=0"
    "-s NODERAWFS=0"
    "-s WASMFS=0"
    "-s EXPORTED_FUNCTIONS='[\"_main\", \"_initializeEngine\", \"_updateEngine\", \"_renderFrame\", \"_shutdownEngine\", \"_getPlatformType\", \"_createPlatform\", \"_getErrorMessage\"]'"
    "-s EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"getValue\", \"setValue\", \"UTF8ToString\", \"stringToUTF8\", \"lengthBytesUTF8\", \"addFunction\"]'"
    "--preload-file ../../assets@/assets"
    "--shell-file ../../src/web/shell.html"
    "-s DISABLE_EXCEPTION_CATCHING=0"
    "-s DEMANGLE_SUPPORT=1"
    "-s GL_PREINITIALIZED_CONTEXT=1"
    "-s USE_WEBGL2=1"
    "-s WEBGL2_BACKWARDS_COMPATIBILITY=1"
    "-s OFFSCREEN_FRAMEBUFFER=1"
    "-s OFFSCREENCANVAS_SUPPORT=1"
    "-s AUDIO_WORKLET=1"
    "-s WEBAUDIO_DEBUG=0"
    "-s PTHREADS=0"
    "-s PTHREAD_POOL_SIZE=0"
    "-s BUILD_AS_WORKER=0"
    "-s MINIMAL_RUNTIME=0"
    "-s ENVIRONMENT='web'"
    "-s TEXTDECODER=1"
    "-s CANVAS_SELECTOR='#canvas'"
    "-s INCOMING_MODULE_JS_API='[\"preRun\", \"onRuntimeInitialized\", \"onAbort\", \"onExit\"]'"
    "--js-library ../../src/web/library.js"
    "-s LEGACY_GL_EMULATION=0"
    "-s GL_UNSAFE_OPTS=0"
    "-s WEBGL_USE_GLES2_COMPAT=0"
    "-s EMULATE_FUNCTION_POINTER_CASTS=0"
    "-s RETAIN_COMPILER_SETTINGS=0"
    "-s GL_TRACK_ERRORS=0"
    "-s GL_DEBUG=0"
    "-s GL_TESTING=0"
    "-s GL_MAX_TEMP_BUFFER_SIZE=16777216"
    "-s GL_MAX_ELEMENTS_VERTICES=65536"
    "-s GL_MAX_ELEMENTS_INDICES=65536"
    "-s GL_VERTEX_ARRAY_BINDING=0"
    "-s GL_ANISOTROPIC_TEXTURE_STRIDE=0"
    "-s BINARYEN_METHOD='native-wasm'"
    "-s WASM_OBJECT_FILES=0"
    "-s WASM_BIGINT=0"
    "-s WASM_WORKERS=0"
    "-s WASM_MEM_MAX=2147483648"
    "-s DEFAULT_TO_CXX=1"
    "-std=c++17"
    # Essential warning suppressions only
    "-Wno-unused-function"
    "-Wno-unused-variable"
    "-Wno-unused-parameter"
    "-Wno-missing-braces"
    "-Wno-writable-strings"
    "-Wno-deprecated-declarations"
    "-Wno-ignored-qualifiers"
)

# Build the engine
echo "🏗️ Building FoundryEngine WASM..."

emcc "${EMSCRIPTEN_FLAGS[@]}" \
    ../../src/main.cpp \
    ../../platforms/web/WebPlatformPAL.cpp \
    -o foundryengine_wasm.js

echo "✅ WASM Build completed successfully!"
echo "📁 Output files:"
echo "   - foundryengine_wasm.js (JavaScript module)"
echo "   - foundryengine_wasm.wasm (WebAssembly binary)"
echo ""
echo "🌐 To use in your web application:"
echo "   <script src='foundryengine_wasm.js'></script>"
echo "   <script>"
echo "     const engine = FoundryEngineWASM();"
echo "     engine.onRuntimeInitialized = () => {"
echo "       console.log('FoundryEngine WASM loaded!');"
echo "     };"
echo "   </script>"
