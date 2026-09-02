@echo off
cd ..

echo -- Installing VulkanSDK
winget install -e --id KhronosGroup.VulkanSDK

echo -- Installing CMake
winget install -e --id Kitware.CMake

echo -- Installing Ninja
winget install -e --id Ninja-build.Ninja

echo -- Installing Git
winget install -e --id Git.Git

echo -- Installing MSVC
winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows10SDK"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Building...
cmake -B build -G "Ninja"
cmake --build build

echo .exe in build/
pause
