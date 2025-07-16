#!/bin/bash

# SPDX-FileCopyrightText: 2024 DeepinScan Team
# SPDX-License-Identifier: LGPL-3.0-or-later

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

echo "Building DeepinScan..."
echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring project..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="/usr" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo "Building project..."
make -j$(nproc)

# Run tests
echo "Running tests..."
make test

echo "Build completed successfully!"
echo "Binaries are in: $BUILD_DIR"
echo ""
echo "To install the library:"
echo "  sudo make install"
echo ""
echo "To run the basic example:"
echo "  ./examples/deepinscan_basic_usage" 