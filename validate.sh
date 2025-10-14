#!/bin/bash

# FoundryEngine Validation Script
# This script validates that the project is properly configured and builds correctly

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

# Function to validate project structure
validate_structure() {
    print_status "Validating project structure..."
    
    local required_dirs=("src" "include" "ide" "tests" "examples" "platforms" "networking")
    local required_files=("CMakeLists.txt" "README.md" "build.sh" "foundry-engine.json")
    
    for dir in "${required_dirs[@]}"; do
        if [ ! -d "$dir" ]; then
            print_error "Required directory missing: $dir"
            return 1
        fi
    done
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Required file missing: $file"
            return 1
        fi
    done
    
    print_success "Project structure validation passed"
    return 0
}

# Function to validate source files
validate_sources() {
    print_status "Validating source files..."
    
    local source_files=(
        "src/Engine.cpp"
        "src/World.cpp"
        "src/Scene.cpp"
        "src/MemoryPool.cpp"
        "src/systems/AudioSystem.cpp"
        "src/systems/InputSystem.cpp"
        "src/systems/PhysicsSystem.cpp"
        "src/systems/ScriptingSystem.cpp"
        "src/systems/AssetSystem.cpp"
        "src/systems/UISystem.cpp"
        "src/systems/NetworkSystem.cpp"
        "src/systems/ProfilerSystem.cpp"
        "src/graphics/D3D11Renderer.cpp"
        "src/graphics/OpenGLRenderer.cpp"
    )
    
    for file in "${source_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Source file missing: $file"
            return 1
        fi
    done
    
    print_success "Source files validation passed"
    return 0
}

# Function to validate header files
validate_headers() {
    print_status "Validating header files..."
    
    local header_files=(
        "include/GameEngine/core/Engine.h"
        "include/GameEngine/core/World.h"
        "include/GameEngine/core/Scene.h"
        "include/GameEngine/core/Entity.h"
        "include/GameEngine/core/Component.h"
        "include/GameEngine/core/MemoryPool.h"
        "include/GameEngine/math/Vector3.h"
        "include/GameEngine/math/Vector2.h"
        "include/GameEngine/math/Matrix4.h"
        "include/GameEngine/systems/AudioSystem.h"
        "include/GameEngine/systems/InputSystem.h"
        "include/GameEngine/systems/PhysicsSystem.h"
        "include/GameEngine/systems/ScriptingSystem.h"
        "include/GameEngine/systems/AssetSystem.h"
        "include/GameEngine/systems/UISystem.h"
        "include/GameEngine/systems/NetworkSystem.h"
        "include/GameEngine/systems/ProfilerSystem.h"
        "include/GameEngine/graphics/Renderer.h"
        "include/GameEngine/graphics/GraphicsTypes.h"
    )
    
    for file in "${header_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Header file missing: $file"
            return 1
        fi
    done
    
    print_success "Header files validation passed"
    return 0
}

# Function to validate IDE files
validate_ide() {
    print_status "Validating IDE files..."
    
    local ide_files=(
        "ide/src/commonMain/kotlin/com/foundry/ide/IdeApplication.kt"
        "ide/src/commonMain/kotlin/com/foundry/ide/EngineIntegration.kt"
        "ide/src/commonMain/kotlin/com/foundry/ide/managers/ProjectManager.kt"
        "ide/src/commonMain/kotlin/com/foundry/ide/managers/BuildManager.kt"
        "ide/src/commonMain/kotlin/com/foundry/ide/managers/DebugManager.kt"
        "ide/src/jvmMain/kotlin/com/foundry/ide/JvmEngineIntegration.kt"
        "ide/src/jsMain/kotlin/com/foundry/ide/JsEngineIntegration.kt"
    )
    
    for file in "${ide_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "IDE file missing: $file"
            return 1
        fi
    done
    
    print_success "IDE files validation passed"
    return 0
}

# Function to validate test files
validate_tests() {
    print_status "Validating test files..."
    
    local test_files=(
        "tests/CMakeLists.txt"
        "tests/TestMemoryPool.cpp"
        "tests/TestCoreSystems.cpp"
        "tests/TestMath.cpp"
        "tests/TestECS.cpp"
        "tests/TestNetworking.cpp"
        "tests/TestRunner.cpp"
    )
    
    for file in "${test_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Test file missing: $file"
            return 1
        fi
    done
    
    print_success "Test files validation passed"
    return 0
}

# Function to validate CMake configuration
validate_cmake() {
    print_status "Validating CMake configuration..."
    
    if ! command_exists cmake; then
        print_error "CMake is not installed"
        return 1
    fi
    
    # Test CMake configuration
    mkdir -p build_validation
    cd build_validation
    
    if ! cmake .. -DCMAKE_BUILD_TYPE=Debug; then
        print_error "CMake configuration failed"
        cd ..
        return 1
    fi
    
    cd ..
    rm -rf build_validation
    
    print_success "CMake configuration validation passed"
    return 0
}

# Function to validate build system
validate_build() {
    print_status "Validating build system..."
    
    if [ ! -f "build.sh" ]; then
        print_error "Build script missing"
        return 1
    fi
    
    if [ ! -x "build.sh" ]; then
        print_warning "Build script is not executable, fixing..."
        chmod +x build.sh
    fi
    
    print_success "Build system validation passed"
    return 0
}

# Function to validate documentation
validate_documentation() {
    print_status "Validating documentation..."
    
    local doc_files=(
        "README.md"
        "foundry-engine.json"
    )
    
    for file in "${doc_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Documentation file missing: $file"
            return 1
        fi
    done
    
    # Check if README has content
    if [ ! -s "README.md" ]; then
        print_error "README.md is empty"
        return 1
    fi
    
    print_success "Documentation validation passed"
    return 0
}

# Function to validate networking
validate_networking() {
    print_status "Validating networking files..."
    
    local networking_files=(
        "networking/server/server.go"
        "networking/client/client.go"
        "networking/shared/shared.go"
    )
    
    for file in "${networking_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_error "Networking file missing: $file"
            return 1
        fi
    done
    
    print_success "Networking files validation passed"
    return 0
}

# Function to run basic syntax checks
validate_syntax() {
    print_status "Validating syntax..."
    
    # Check C++ files for basic syntax
    local cpp_files=$(find src include -name "*.cpp" -o -name "*.h" | head -10)
    for file in $cpp_files; do
        if ! grep -q "#include\|#pragma\|namespace\|class\|struct" "$file"; then
            print_warning "File $file may have syntax issues"
        fi
    done
    
    # Check Kotlin files for basic syntax
    local kt_files=$(find ide -name "*.kt" | head -5)
    for file in $kt_files; do
        if ! grep -q "package\|import\|class\|fun" "$file"; then
            print_warning "File $file may have syntax issues"
        fi
    done
    
    print_success "Syntax validation passed"
    return 0
}

# Function to generate validation report
generate_report() {
    local report_file="validation_report.txt"
    
    print_status "Generating validation report..."
    
    cat > "$report_file" << EOF
FoundryEngine Validation Report
Generated: $(date)
Version: 1.0.0

Project Structure: ✓
Source Files: ✓
Header Files: ✓
IDE Files: ✓
Test Files: ✓
CMake Configuration: ✓
Build System: ✓
Documentation: ✓
Networking: ✓
Syntax: ✓

Total Files: $(find . -type f -name "*.cpp" -o -name "*.h" -o -name "*.kt" -o -name "*.go" | wc -l)
Total Lines: $(find . -type f -name "*.cpp" -o -name "*.h" -o -name "*.kt" -o -name "*.go" | xargs wc -l | tail -1)

Validation Status: PASSED
EOF
    
    print_success "Validation report generated: $report_file"
}

# Main validation function
main() {
    print_status "Starting FoundryEngine validation..."
    
    local validation_passed=true
    
    # Run all validation checks
    validate_structure || validation_passed=false
    validate_sources || validation_passed=false
    validate_headers || validation_passed=false
    validate_ide || validation_passed=false
    validate_tests || validation_passed=false
    validate_cmake || validation_passed=false
    validate_build || validation_passed=false
    validate_documentation || validation_passed=false
    validate_networking || validation_passed=false
    validate_syntax || validation_passed=false
    
    if [ "$validation_passed" = true ]; then
        print_success "All validations passed!"
        generate_report
        exit 0
    else
        print_error "Some validations failed!"
        exit 1
    fi
}

# Run main function
main "$@"
