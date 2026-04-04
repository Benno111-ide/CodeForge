#!/usr/bin/env bash
clear

# CodeForge - Professional C++ IDE Build Script
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building CodeForge - Professional C++ IDE"
echo "=========================================="

# Clean previous build
echo "Cleaning previous build..."
sudo rm -rf build
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the application
echo "Building application..."
cmake --build . -j$(nproc)

echo ""
echo "Build completed successfully!"
echo ""
echo "To run CodeForge:"
echo "  cd build"
echo "  ./bin/CodeForge"
echo ""
echo "CodeForge provides a professional C++ development environment with:"
echo "  - Advanced syntax-highlighted code editor"
echo "  - Real-time compilation with intelligent error detection"
echo "  - Integrated project management and build system"
echo "  - Cross-platform compatibility"
echo "  - Built-in code examples and templates"
./bin/CodeForge