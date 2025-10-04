#!/bin/bash

# Foundry IDE Build Script
# Builds the complete IDE for all platforms

set -e

echo "🚀 Foundry IDE Build Script"
echo "=========================="

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

# Check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."

    # Check Java
    if ! command -v java &> /dev/null; then
        print_error "Java is not installed. Please install Java 17 or higher."
        exit 1
    fi

    JAVA_VERSION=$(java -version 2>&1 | head -n1 | cut -d'"' -f2 | sed 's/^1\.//' | cut -d'.' -f1)
    if [ "$JAVA_VERSION" -lt "17" ]; then
        print_error "Java 17 or higher is required. Current version: $JAVA_VERSION"
        exit 1
    fi
    print_success "Java $JAVA_VERSION found"

    # Check Gradle
    if ! command -v gradle &> /dev/null && ! command -v ./gradlew &> /dev/null; then
        print_error "Gradle is not installed. Please install Gradle or use the included wrapper."
        exit 1
    fi
    print_success "Gradle found"

    # Check Node.js (for web build)
    if command -v node &> /dev/null; then
        NODE_VERSION=$(node --version | cut -d'v' -f2 | cut -d'.' -f1)
        if [ "$NODE_VERSION" -lt "16" ]; then
            print_warning "Node.js version $NODE_VERSION is below recommended 16+. Web build may fail."
        else
            print_success "Node.js $(node --version) found"
        fi
    else
        print_warning "Node.js not found. Web build will be skipped."
    fi

    # Check CMake (for JNI build)
    if command -v cmake &> /dev/null; then
        print_success "CMake $(cmake --version | head -n1) found"
    else
        print_warning "CMake not found. JNI bridge build will be skipped."
    fi
}

# Build JNI bridge
build_jni() {
    if [ ! -d "jni" ]; then
        print_warning "JNI directory not found. Skipping JNI build."
        return 0
    fi

    if ! command -v cmake &> /dev/null; then
        print_warning "CMake not found. Skipping JNI build."
        return 0
    fi

    print_status "Building JNI bridge..."

    cd jni

    # Create build directory
    mkdir -p build
    cd build

    # Configure and build
    cmake ..
    make

    if [ $? -eq 0 ]; then
        print_success "JNI bridge built successfully"
        cd ../..
        return 0
    else
        print_error "JNI bridge build failed"
        cd ../..
        return 1
    fi
}

# Build desktop version
build_desktop() {
    print_status "Building desktop version (JVM)..."

    if command -v gradle &> /dev/null; then
        gradle jvmJar
    else
        ./gradlew jvmJar
    fi

    if [ $? -eq 0 ]; then
        print_success "Desktop version built successfully"
        return 0
    else
        print_error "Desktop build failed"
        return 1
    fi
}

# Build web version
build_web() {
    if ! command -v node &> /dev/null; then
        print_warning "Node.js not found. Skipping web build."
        return 0
    fi

    print_status "Building web version (JavaScript)..."

    if command -v gradle &> /dev/null; then
        gradle jsBrowserProductionWebpack
    else
        ./gradlew jsBrowserProductionWebpack
    fi

    if [ $? -eq 0 ]; then
        print_success "Web version built successfully"
        return 0
    else
        print_error "Web build failed"
        return 1
    fi
}

# Create distribution packages
create_packages() {
    print_status "Creating distribution packages..."

    # Create directories
    mkdir -p dist/desktop
    mkdir -p dist/web

    # Copy desktop build
    if [ -d "build/libs" ]; then
        cp build/libs/* dist/desktop/ 2>/dev/null || true
    fi

    # Copy web build
    if [ -d "build/distributions" ]; then
        cp -r build/distributions/* dist/web/ 2>/dev/null || true
    fi

    # Copy JNI libraries
    if [ -d "jni/build/lib" ]; then
        cp -r jni/build/lib/* dist/desktop/ 2>/dev/null || true
    fi

    # Create run scripts
    create_run_scripts

    print_success "Distribution packages created in dist/"
}

# Create run scripts
create_run_scripts() {
    # Desktop run script
    cat > dist/desktop/run-ide.sh << 'EOF'
#!/bin/bash
# Foundry IDE Desktop Launcher

echo "Starting Foundry IDE..."
echo "======================"

# Set Java options
JAVA_OPTS="-Xmx2g -Xms512m"

# Find the JAR file
JAR_FILE=$(ls *.jar | head -n1)

if [ -z "$JAR_FILE" ]; then
    echo "Error: No JAR file found!"
    exit 1
fi

# Run the IDE
java $JAVA_OPTS -jar "$JAR_FILE"
EOF

    chmod +x dist/desktop/run-ide.sh

    # Web run script
    cat > dist/web/run-web.sh << 'EOF'
#!/bin/bash
# Foundry IDE Web Launcher

echo "Starting Foundry IDE Web Server..."
echo "=================================="

# Check if Python is available for simple HTTP server
if command -v python3 &> /dev/null; then
    echo "Starting web server on http://localhost:8080"
    echo "Open your browser and navigate to: http://localhost:8080"
    cd "$(dirname "$0")"
    python3 -m http.server 8080
elif command -v python &> /dev/null; then
    echo "Starting web server on http://localhost:8080"
    echo "Open your browser and navigate to: http://localhost:8080"
    cd "$(dirname "$0")"
    python -m SimpleHTTPServer 8080
else
    echo "Python not found. Please serve the web files manually."
    echo "Open index.html in your browser to run the web IDE."
fi
EOF

    chmod +x dist/web/run-web.sh

    # Windows batch scripts
    cat > dist/desktop/run-ide.bat << 'EOF'
@echo off
echo Starting Foundry IDE...
echo ======================

set JAVA_OPTS=-Xmx2g -Xms512m

for %%f in (*.jar) do set JAR_FILE=%%f

if "%JAR_FILE%"=="" (
    echo Error: No JAR file found!
    pause
    exit /b 1
)

java %JAVA_OPTS% -jar "%JAR_FILE%"
EOF

    cat > dist/web/run-web.bat << 'EOF'
@echo off
echo Starting Foundry IDE Web Server...
echo ==================================
echo.
echo Open your browser and navigate to: http://localhost:8080
echo.

cd /d "%~dp0"

python -m http.server 8080 2>nul
if errorlevel 1 (
    echo Python not found. Please install Python or serve the files manually.
    echo Open index.html in your browser to run the web IDE.
    pause
)
EOF
}

# Main build process
main() {
    check_prerequisites

    echo
    print_status "Starting build process..."

    # Build JNI bridge
    build_jni

    # Build desktop version
    build_desktop

    # Build web version
    build_web

    # Create packages
    create_packages

    echo
    print_success "Build completed successfully!"
    echo
    echo "Desktop IDE:"
    echo "  - Location: dist/desktop/"
    echo "  - Run with: ./run-ide.sh (Linux/Mac) or run-ide.bat (Windows)"
    echo
    echo "Web IDE:"
    echo "  - Location: dist/web/"
    echo "  - Run with: ./run-web.sh (Linux/Mac) or run-web.bat (Windows)"
    echo "  - Open index.html in your browser"
    echo
    echo "For more information, see ide/README.md"
}

# Run main function
main "$@"
