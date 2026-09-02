cmake_minimum_required(VERSION 3.28)
project(Crucible CXX)

if(MSVC)
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
  set(CMAKE_CXX_STANDARD 26)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)

## getting pkgs with cmake 
include(FetchContent)
FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG v3.2.1
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator
)

include(FetchContent)
FetchContent_Declare(
  CGLTF
  GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
  GIT_TAG v1.15
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/cgltf
)

include(FetchContent)
FetchContent_Declare(
  GLM
  GIT_REPOSITORY https://github.com/g-truc/glm.git 
  GIT_TAG 1.0.3
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/glm
)

include(FetchContent)
FetchContent_Declare(
  GLFW
  GIT_REPOSITORY https://github.com/glfw/glfw.git 
  GIT_TAG 3.5.1
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/glfw
)

include(FetchContent)
FetchContent_Declare(
  STB
  GIT_REPOSITORY https://github.com/nothings/stb.git 
  GIT_TAG master
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/stb
)

FetchContent_MakeAvailable(VulkanMemoryAllocator CGLTF GLM GLFW STB)

add_executable(Crucible
  ## main
  src/main.cc

  ## core 
  src/core/i_render.cc
  ## vulkan
  src/vulkan/window.cc
  src/vulkan/buffer.cc
  src/vulkan/context.cc
  src/vulkan/command.cc
  src/vulkan/pipeline.cc
  src/vulkan/descriptor.cc

  ## Impl files
  ext/vma_impl.cc
  ext/stb_impl.cc
  ext/cgltf_impl.cc
)

## window 32-bit support
if(WIN32)
    target_compile_definitions(Crucible PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

## all exeternal libs and source code include 
target_include_directories(Crucible SYSTEM PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${Vulkan_INCLUDE_DIRS}
  ${glm_SOURCE_DIR}
  ${cgltf_SOURCE_DIR}
  ${glfw_SOURCE_DIR}/include
  ${stb_SOURCE_DIR}
  ${vulkanmemoryallocator_SOURCE_DIR}/include
  ${vulkanmemoryallocator_SOURCE_DIR}/src
)

## precompiled header
target_precompile_headers(Crucible PRIVATE
  "src/core/pch.h"
)

## linking libs
target_link_libraries(Crucible PRIVATE
  glfw
  VulkanMemoryAllocator
  ${Vulkan_LIBRARIES}
)

## defs for paths
target_compile_definitions(Crucible PRIVATE
  SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/"
  MODEL_PATH="${CMAKE_BINARY_DIR}/assets/models/"
  TEXTURE_PATH="${CMAKE_BINARY_DIR}/assets/textures/"
)

## shader compile
add_custom_target(assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_SOURCE_DIR}/assets/models"
  "${CMAKE_BINARY_DIR}/assets/models"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_SOURCE_DIR}/assets/textures"
  "${CMAKE_BINARY_DIR}/assets/textures"
  DEPENDS ${CMAKE_SOURCE_DIR}/assets/models ${CMAKE_SOURCE_DIR}/assets/textures
  COMMENT "Copying assets..."
)

find_program(GLSLC_BIN glslc
  HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
  REQUIRED
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/shaders/shader_vert.spv ${CMAKE_BINARY_DIR}/shaders/shader_frag.spv
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
  COMMAND ${GLSLC_BIN} -fshader-stage=vertex ${CMAKE_SOURCE_DIR}/assets/shaders/shader_vert.glsl
  -o ${CMAKE_BINARY_DIR}/shaders/shader_vert.spv
  COMMAND ${GLSLC_BIN} -fshader-stage=fragment ${CMAKE_SOURCE_DIR}/assets/shaders/shader_frag.glsl -o ${CMAKE_BINARY_DIR}/shaders/shader_frag.spv
  DEPENDS ${CMAKE_SOURCE_DIR}/assets/shaders/shader_vert.glsl ${CMAKE_SOURCE_DIR}/assets/shaders/shader_frag.glsl
)
add_custom_target(shaders DEPENDS ${CMAKE_BINARY_DIR}/shaders/shader_vert.spv ${CMAKE_BINARY_DIR}/shaders/shader_frag.spv)

## dep
add_dependencies(Crucible shaders)
add_dependencies(Crucible assets)

