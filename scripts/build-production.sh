#!/bin/bash

# FoundryEngine Production Build Script
# Builds the complete engine for all target platforms with optimization

set -e

echo "🚀 FoundryEngine Production Build System"
echo "========================================"

# Configuration
BUILD_DIR="build/production"
DIST_DIR="dist"
VERSION=$(cat VERSION 2>/dev/null || echo "1.0.0")
BUILD_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

# Build configurations
PLATFORMS=("windows" "linux" "macos" "android" "ios" "web")
ARCHITECTURES=("x64" "arm64")
BUILD_TYPES=("Release" "Debug")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking build prerequisites..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake not found. Please install CMake 3.16 or later."
        exit 1
    fi
    
    # Check compiler
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if ! command -v g++ &> /dev/null; then
            log_error "GCC not found. Please install GCC 10 or later."
            exit 1
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        if ! command -v clang++ &> /dev/null; then
            log_error "Clang not found. Please install Xcode command line tools."
            exit 1
        fi
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        if ! command -v cl &> /dev/null; then
            log_error "MSVC not found. Please install Visual Studio 2019 or later."
            exit 1
        fi
    fi
    
    # Check Emscripten for web builds
    if ! command -v emcc &> /dev/null; then
        log_warning "Emscripten not found. Web builds will be skipped."
        WEB_BUILD_AVAILABLE=false
    else
        WEB_BUILD_AVAILABLE=true
    fi
    
    # Check Android SDK for Android builds
    if [[ -z "$ANDROID_HOME" ]]; then
        log_warning "ANDROID_HOME not set. Android builds will be skipped."
        ANDROID_BUILD_AVAILABLE=false
    else
        ANDROID_BUILD_AVAILABLE=true
    fi
    
    log_success "Prerequisites check completed"
}

# Create build directories
setup_directories() {
    log_info "Setting up build directories..."
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$DIST_DIR"
    
    for platform in "${PLATFORMS[@]}"; do
        mkdir -p "$BUILD_DIR/$platform"
        mkdir -p "$DIST_DIR/$platform"
    done
    
    log_success "Build directories created"
}

# Build for Windows
build_windows() {
    log_info "Building for Windows..."
    
    local build_type=${1:-Release}
    local arch=${2:-x64}
    
    local build_path="$BUILD_DIR/windows/$build_type-$arch"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # Configure with CMake
    cmake ../../../../ \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_GENERATOR_PLATFORM="$arch" \
        -DENABLE_PHYSICS=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_NETWORKING=ON \
        -DENABLE_SCRIPTING=ON \
        -DENABLE_PROFILING=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=ON
    
    # Build
    cmake --build . --config "$build_type" --parallel
    
    # Package
    cpack -C "$build_type"
    
    # Copy to distribution
    cp *.exe ../../../../"$DIST_DIR/windows/"
    cp *.dll ../../../../"$DIST_DIR/windows/" 2>/dev/null || true
    cp *.lib ../../../../"$DIST_DIR/windows/" 2>/dev/null || true
    
    cd ../../../../..
    log_success "Windows build completed"
}

# Build for Linux
build_linux() {
    log_info "Building for Linux..."
    
    local build_type=${1:-Release}
    local arch=${2:-x64}
    
    local build_path="$BUILD_DIR/linux/$build_type-$arch"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # Configure with CMake
    cmake ../../../../ \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DENABLE_PHYSICS=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_NETWORKING=ON \
        -DENABLE_SCRIPTING=ON \
        -DENABLE_PROFILING=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=ON
    
    # Build
    cmake --build . --parallel
    
    # Package
    cpack
    
    # Copy to distribution
    cp *.deb ../../../../"$DIST_DIR/linux/" 2>/dev/null || true
    cp *.rpm ../../../../"$DIST_DIR/linux/" 2>/dev/null || true
    cp *.tar.gz ../../../../"$DIST_DIR/linux/" 2>/dev/null || true
    
    cd ../../../../..
    log_success "Linux build completed"
}

# Build for macOS
build_macos() {
    log_info "Building for macOS..."
    
    local build_type=${1:-Release}
    local arch=${2:-x64}
    
    local build_path="$BUILD_DIR/macos/$build_type-$arch"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # Configure with CMake
    cmake ../../../../ \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_SYSTEM_NAME=Darwin \
        -DCMAKE_OSX_ARCHITECTURES="$arch" \
        -DENABLE_PHYSICS=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_NETWORKING=ON \
        -DENABLE_SCRIPTING=ON \
        -DENABLE_PROFILING=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=ON
    
    # Build
    cmake --build . --parallel
    
    # Package
    cpack
    
    # Copy to distribution
    cp *.dmg ../../../../"$DIST_DIR/macos/" 2>/dev/null || true
    cp *.tar.gz ../../../../"$DIST_DIR/macos/" 2>/dev/null || true
    
    cd ../../../../..
    log_success "macOS build completed"
}

# Build for Android
build_android() {
    if [[ "$ANDROID_BUILD_AVAILABLE" != "true" ]]; then
        log_warning "Skipping Android build - ANDROID_HOME not set"
        return
    fi
    
    log_info "Building for Android..."
    
    local build_type=${1:-Release}
    local arch=${2:-arm64-v8a}
    
    local build_path="$BUILD_DIR/android/$build_type-$arch"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # Configure with CMake
    cmake ../../../../ \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_SYSTEM_NAME=Android \
        -DCMAKE_ANDROID_ARCH_ABI="$arch" \
        -DCMAKE_ANDROID_STL_TYPE=c++_shared \
        -DENABLE_PHYSICS=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_NETWORKING=ON \
        -DENABLE_SCRIPTING=ON \
        -DENABLE_PROFILING=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=ON
    
    # Build
    cmake --build . --parallel
    
    # Package
    cpack
    
    # Copy to distribution
    cp *.apk ../../../../"$DIST_DIR/android/" 2>/dev/null || true
    cp *.aab ../../../../"$DIST_DIR/android/" 2>/dev/null || true
    
    cd ../../../../..
    log_success "Android build completed"
}

# Build for iOS
build_ios() {
    log_info "Building for iOS..."
    
    local build_type=${1:-Release}
    local arch=${2:-arm64}
    
    local build_path="$BUILD_DIR/ios/$build_type-$arch"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # Configure with CMake
    cmake ../../../../ \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_ARCHITECTURES="$arch" \
        -DENABLE_PHYSICS=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_NETWORKING=ON \
        -DENABLE_SCRIPTING=ON \
        -DENABLE_PROFILING=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=ON
    
    # Build
    cmake --build . --parallel
    
    # Package
    cpack
    
    # Copy to distribution
    cp *.ipa ../../../../"$DIST_DIR/ios/" 2>/dev/null || true
    cp *.tar.gz ../../../../"$DIST_DIR/ios/" 2>/dev/null || true
    
    cd ../../../../..
    log_success "iOS build completed"
}

# Build for Web
build_web() {
    if [[ "$WEB_BUILD_AVAILABLE" != "true" ]]; then
        log_warning "Skipping Web build - Emscripten not available"
        return
    fi
    
    log_info "Building for Web..."
    
    local build_type=${1:-Release}
    local build_path="$BUILD_DIR/web/$build_type"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # Configure with Emscripten
    emcmake cmake ../../../../ \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DCMAKE_SYSTEM_NAME=Emscripten \
        -DENABLE_PHYSICS=ON \
        -DENABLE_AUDIO=ON \
        -DENABLE_NETWORKING=ON \
        -DENABLE_SCRIPTING=ON \
        -DENABLE_PROFILING=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTS=OFF
    
    # Build
    emmake make -j$(nproc)
    
    # Copy to distribution
    cp *.js ../../../../"$DIST_DIR/web/" 2>/dev/null || true
    cp *.wasm ../../../../"$DIST_DIR/web/" 2>/dev/null || true
    cp *.html ../../../../"$DIST_DIR/web/" 2>/dev/null || true
    
    cd ../../../../..
    log_success "Web build completed"
}

# Build IDE
build_ide() {
    log_info "Building IDE..."
    
    cd ide
    
    # Build with Gradle
    ./gradlew build
    
    # Copy to distribution
    cp build/libs/*.jar ../"$DIST_DIR/ide/" 2>/dev/null || true
    
    cd ..
    log_success "IDE build completed"
}

# Run tests
run_tests() {
    log_info "Running tests..."
    
    for platform in "${PLATFORMS[@]}"; do
        if [[ -d "$BUILD_DIR/$platform" ]]; then
            log_info "Running tests for $platform..."
            
            # Find test executables
            find "$BUILD_DIR/$platform" -name "*Test*" -type f -executable | while read test_file; do
                log_info "Running test: $test_file"
                "$test_file" || log_warning "Test failed: $test_file"
            done
        fi
    done
    
    log_success "Tests completed"
}

# Generate documentation
generate_documentation() {
    log_info "Generating documentation..."
    
    # Generate API documentation
    if command -v doxygen &> /dev/null; then
        doxygen Doxyfile
        log_success "API documentation generated"
    else
        log_warning "Doxygen not found. Skipping API documentation."
    fi
    
    # Generate user manual
    if command -v pandoc &> /dev/null; then
        pandoc docs/user-manual.md -o "$DIST_DIR/FoundryEngine-UserManual.pdf"
        log_success "User manual generated"
    else
        log_warning "Pandoc not found. Skipping user manual generation."
    fi
    
    log_success "Documentation generation completed"
}

# Create distribution packages
create_distribution() {
    log_info "Creating distribution packages..."
    
    # Create version info
    cat > "$DIST_DIR/version.json" << EOF
{
    "version": "$VERSION",
    "buildDate": "$BUILD_DATE",
    "platforms": [
EOF
    
    local first=true
    for platform in "${PLATFORMS[@]}"; do
        if [[ -d "$DIST_DIR/$platform" ]] && [[ -n "$(ls -A "$DIST_DIR/$platform" 2>/dev/null)" ]]; then
            if [[ "$first" == "true" ]]; then
                first=false
            else
                echo "," >> "$DIST_DIR/version.json"
            fi
            echo "        \"$platform\"" >> "$DIST_DIR/version.json"
        fi
    done
    
    cat >> "$DIST_DIR/version.json" << EOF
    ],
    "features": [
        "ECS Architecture",
        "Cross-Platform Graphics",
        "Physics Simulation",
        "Audio System",
        "Networking",
        "Scripting",
        "Asset Pipeline",
        "WebAssembly Support"
    ]
}
EOF
    
    # Create checksums
    find "$DIST_DIR" -type f -name "*.exe" -o -name "*.dll" -o -name "*.so" -o -name "*.dylib" -o -name "*.wasm" -o -name "*.apk" -o -name "*.ipa" | while read file; do
        if command -v sha256sum &> /dev/null; then
            sha256sum "$file" >> "$DIST_DIR/checksums.txt"
        elif command -v shasum &> /dev/null; then
            shasum -a 256 "$file" >> "$DIST_DIR/checksums.txt"
        fi
    done
    
    log_success "Distribution packages created"
}

# Main build function
main() {
    local build_platform=""
    local build_type="Release"
    local build_arch="x64"
    local skip_tests=false
    local skip_docs=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --platform)
                build_platform="$2"
                shift 2
                ;;
            --type)
                build_type="$2"
                shift 2
                ;;
            --arch)
                build_arch="$2"
                shift 2
                ;;
            --skip-tests)
                skip_tests=true
                shift
                ;;
            --skip-docs)
                skip_docs=true
                shift
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --platform PLATFORM    Build for specific platform (windows, linux, macos, android, ios, web)"
                echo "  --type TYPE           Build type (Release, Debug)"
                echo "  --arch ARCH           Architecture (x64, arm64)"
                echo "  --skip-tests          Skip running tests"
                echo "  --skip-docs           Skip documentation generation"
                echo "  --help                Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    # Start build process
    log_info "Starting FoundryEngine production build"
    log_info "Version: $VERSION"
    log_info "Build Date: $BUILD_DATE"
    
    check_prerequisites
    setup_directories
    
    # Build for specific platform or all platforms
    if [[ -n "$build_platform" ]]; then
        case "$build_platform" in
            windows)
                build_windows "$build_type" "$build_arch"
                ;;
            linux)
                build_linux "$build_type" "$build_arch"
                ;;
            macos)
                build_macos "$build_type" "$build_arch"
                ;;
            android)
                build_android "$build_type" "$build_arch"
                ;;
            ios)
                build_ios "$build_type" "$build_arch"
                ;;
            web)
                build_web "$build_type"
                ;;
            *)
                log_error "Unknown platform: $build_platform"
                exit 1
                ;;
        esac
    else
        # Build for all platforms
        build_windows "$build_type" "$build_arch"
        build_linux "$build_type" "$build_arch"
        build_macos "$build_type" "$build_arch"
        build_android "$build_type" "$build_arch"
        build_ios "$build_type" "$build_arch"
        build_web "$build_type"
        build_ide
    fi
    
    # Run tests
    if [[ "$skip_tests" != "true" ]]; then
        run_tests
    fi
    
    # Generate documentation
    if [[ "$skip_docs" != "true" ]]; then
        generate_documentation
    fi
    
    # Create distribution
    create_distribution
    
    log_success "Production build completed successfully!"
    log_info "Distribution files are available in: $DIST_DIR"
}

# Run main function
main "$@"
