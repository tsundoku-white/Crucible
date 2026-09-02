#!/usr/bin/env bash
# One-click build for Crucible (Linux, Ninja + Clang)
set -e

BUILD_DIR="build"
BUILD_TYPE="${1:-Debug}"

cd ..

fail() { echo "ERROR: $1" >&2; exit 1; }

command -v ninja >/dev/null 2>&1 || fail "ninja not found. Install: sudo pacman -S ninja  (Arch)  /  sudo apt install ninja-build  (Debian/Ubuntu)"
command -v clang++ >/dev/null 2>&1 || fail "clang++ not found. Install: sudo pacman -S clang  (Arch)  /  sudo apt install clang  (Debian/Ubuntu)"

if [ -z "$VULKAN_SDK" ]; then
    command -v glslc >/dev/null 2>&1 || fail "No VULKAN_SDK env var and glslc not on PATH. Install the Vulkan SDK: sudo pacman -S vulkan-devel shaderc  (Arch)  or from https://vulkan.lunarg.com/"
    echo "Note: VULKAN_SDK not set, but glslc found on PATH — continuing."
fi

echo "Configuring (Ninja + Clang, ${BUILD_TYPE})..."
cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "Building..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo ""
echo "Build complete: $BUILD_DIR/Crucible"
