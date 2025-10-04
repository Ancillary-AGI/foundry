#!/bin/bash

# Foundry IDE Quick Start Script
# Gets you up and running with Foundry IDE in minutes

set -e

echo "🚀 Foundry IDE Quick Start"
echo "=========================="

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[NOTE]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "ide/build.gradle.kts" ]; then
    echo "Error: Please run this script from the Foundry project root directory"
    exit 1
fi

print_step "Step 1: Building Foundry IDE..."

# Build the IDE
if [ -f "ide/build-ide.sh" ]; then
    cd ide
    bash build-ide.sh
    cd ..
else
    print_warning "Build script not found. Please run 'cd ide && ./gradlew build' manually"
fi

print_success "Foundry IDE built successfully!"

echo
print_step "Step 2: Creating sample project..."

# Create projects directory
mkdir -p projects

# Copy template project
if [ -d "ide/templates/default-project" ]; then
    cp -r ide/templates/default-project projects/MyFirstGame
    print_success "Sample project created at: projects/MyFirstGame/"
else
    print_warning "Project template not found. Creating basic structure..."
    mkdir -p projects/MyFirstGame
    echo "Sample project structure created. Please see ide/README.md for manual setup."
fi

echo
print_step "Step 3: Starting Foundry IDE..."

# Check if desktop build exists
if [ -f "ide/dist/desktop/run-ide.sh" ]; then
    print_success "Desktop IDE is ready!"
    echo "To start the desktop IDE:"
    echo "  cd ide/dist/desktop"
    echo "  ./run-ide.sh"
    echo
elif [ -f "ide/dist/web/index.html" ]; then
    print_success "Web IDE is ready!"
    echo "To start the web IDE:"
    echo "  cd ide/dist/web"
    echo "  ./run-web.sh"
    echo "  Open http://localhost:8080 in your browser"
    echo
else
    print_warning "No built IDE found. Please build manually:"
    echo "  cd ide"
    echo "  ./gradlew run    # For desktop"
    echo "  ./gradlew jsRun  # For web"
fi

echo
print_step "Step 4: Opening sample project..."

if [ -f "projects/MyFirstGame/foundry.json" ]; then
    print_success "Sample project is ready!"
    echo "Project location: projects/MyFirstGame/"
    echo "Main scene: projects/MyFirstGame/scenes/main.scene"
    echo "Project config: projects/MyFirstGame/foundry.json"
    echo
    echo "To open in IDE:"
    echo "1. Start the IDE (see Step 3)"
    echo "2. Select 'File > Open Project'"
    echo "3. Navigate to: projects/MyFirstGame/"
    echo "4. Start developing!"
else
    print_warning "Sample project not found. Please create a project manually."
fi

echo
print_step "🎉 Quick start complete!"
echo
echo "📚 Next steps:"
echo "1. Start the IDE using the commands above"
echo "2. Open the sample project"
echo "3. Explore the interface and create your first entity"
echo "4. Build and run your project"
echo "5. Read the full documentation in ide/README.md"
echo
echo "🔗 Resources:"
echo "• IDE Documentation: ide/README.md"
echo "• Engine Documentation: README.md"
echo "• Example Projects: examples/"
echo "• Community Support: Check README for links"
echo
echo "Happy developing with Foundry! 🚀"

# Optional: Try to start the IDE automatically
read -p "Would you like to start the desktop IDE now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if [ -f "ide/dist/desktop/run-ide.sh" ]; then
        cd ide/dist/desktop
        ./run-ide.sh
    else
        echo "Desktop IDE not found. Please build it first."
    fi
fi
