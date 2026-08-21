#!/usr/bin/env bash
set -euo pipefail

echo -e "Install Vulkan Packages"

common_pkgs=(
    vulkan-icd-loader
    vulkan-headers
    vulkan-tools
    vulkan-validation-layers
    glfw
    shaderc
    spirv-tools
    ninja
    cmake
    git
)
 
sudo pacman -S --needed "${common_pkgs[@]}"
 
# Detect GPU vendor(s)
gpu_info=$(lspci | grep -E "VGA|3D")
echo "Detected GPU(s):"
echo "$gpu_info"
 
driver_pkgs=()
 
if echo "$gpu_info" | grep -qi nvidia; then
    echo "-> NVIDIA GPU detected"
    driver_pkgs+=(nvidia-utils)
fi
 
if echo "$gpu_info" | grep -qi amd; then
    echo "-> AMD GPU detected"
    driver_pkgs+=(vulkan-radeon mesa)
fi
 
if echo "$gpu_info" | grep -qi intel; then
    echo "-> Intel GPU detected"
    driver_pkgs+=(vulkan-intel mesa)
fi
 
if [ ${#driver_pkgs[@]} -eq 0 ]; then
    echo "No known GPU vendor detected. Skipping driver install."
    echo "You may need to install the appropriate Vulkan driver package manually."
else
    echo "Installing driver packages: ${driver_pkgs[*]}"
    sudo pacman -S --needed "${driver_pkgs[@]}"
fi

echo -e "\nfinished."
