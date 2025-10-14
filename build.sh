#!/bin/bash

# FoundryEngine Build Script
# This script builds the FoundryEngine for different platforms

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    if ! command_exists cmake; then
        print_error "CMake is required but not installed"
        exit 1
    fi
    
    if ! command_exists make && ! command_exists ninja; then
        print_error "Make or Ninja is required but not installed"
        exit 1
    fi
    
    print_success "Prerequisites check passed"
}

# Function to build for native platform
build_native() {
    local build_type=${1:-Release}
    local build_dir="build_${build_type,,}"
    
    print_status "Building for native platform (${build_type})..."
    
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    cmake .. -DCMAKE_BUILD_TYPE="$build_type" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    if command_exists ninja; then
        ninja
    else
        make -j$(nproc)
    fi
    
    cd ..
    print_success "Native build completed"
}

# Function to build for WebAssembly
build_web() {
    print_status "Building for WebAssembly..."
    
    if ! command_exists emcmake; then
        print_error "Emscripten is required for WebAssembly builds"
        print_status "Please install Emscripten: https://emscripten.org/docs/getting_started/downloads.html"
        exit 1
    fi
    
    mkdir -p build_web
    cd build_web
    
    emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
    emmake make -j$(nproc)
    
    cd ..
    print_success "WebAssembly build completed"
}

# Function to build for Android
build_android() {
    print_status "Building for Android..."
    
    if [ -z "$ANDROID_NDK" ]; then
        print_error "ANDROID_NDK environment variable is not set"
        exit 1
    fi
    
    mkdir -p build_android
    cd build_android
    
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-21 \
        -DCMAKE_BUILD_TYPE=Release
    
    make -j$(nproc)
    
    cd ..
    print_success "Android build completed"
}

# Function to build IDE
build_ide() {
    print_status "Building IDE..."
    
    if ! command_exists gradle; then
        print_error "Gradle is required for IDE builds"
        exit 1
    fi
    
    cd ide
    ./gradlew build
    cd ..
    
    print_success "IDE build completed"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    if [ -d "build_release" ]; then
        cd build_release
        ctest --output-on-failure
        cd ..
        print_success "Tests completed"
    else
        print_warning "No build directory found. Please build the project first."
    fi
}

# Function to clean build directories
clean() {
    print_status "Cleaning build directories..."
    
    rm -rf build_*
    rm -rf ide/build
    
    print_success "Clean completed"
}

# Function to show help
show_help() {
    echo "FoundryEngine Build Script"
    echo ""
    echo "Usage: $0 [OPTIONS] [TARGET]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -c, --clean    Clean build directories"
    echo "  -t, --test     Run tests"
    echo "  -j, --jobs N   Number of parallel jobs (default: auto)"
    echo ""
    echo "Targets:"
    echo "  native         Build for native platform (default)"
    echo "  web            Build for WebAssembly"
    echo "  android        Build for Android"
    echo "  ide            Build the IDE"
    echo "  all            Build everything"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build for native platform"
    echo "  $0 web               # Build for WebAssembly"
    echo "  $0 android           # Build for Android"
    echo "  $0 ide               # Build the IDE"
    echo "  $0 all               # Build everything"
    echo "  $0 -c                # Clean build directories"
    echo "  $0 -t                # Run tests"
}

# Main script logic
main() {
    local target="native"
    local clean_build=false
    local run_tests_flag=false
    local jobs=$(nproc)
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_build=true
                shift
                ;;
            -t|--test)
                run_tests_flag=true
                shift
                ;;
            -j|--jobs)
                jobs="$2"
                shift 2
                ;;
            native|web|android|ide|all)
                target="$1"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Clean if requested
    if [ "$clean_build" = true ]; then
        clean
        if [ "$target" = "clean" ]; then
            exit 0
        fi
    fi
    
    # Check prerequisites
    check_prerequisites
    
    # Build based on target
    case $target in
        native)
            build_native
            ;;
        web)
            build_web
            ;;
        android)
            build_android
            ;;
        ide)
            build_ide
            ;;
        all)
            build_native
            build_web
            build_ide
            ;;
        *)
            print_error "Unknown target: $target"
            show_help
            exit 1
            ;;
    esac
    
    # Run tests if requested
    if [ "$run_tests_flag" = true ]; then
        run_tests
    fi
    
    print_success "Build process completed successfully!"
}

# Run main function with all arguments
main "$@"
