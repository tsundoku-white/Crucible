cmake_minimum_required(VERSION 3.20)
project(Crucible CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_GENERATOR Ninja)

find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG v3.2.1
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

find_path(CGLTF_INCLUDE_DIR cgltf.h
    PATHS ${CMAKE_SOURCE_DIR}/ext/cgltf
    REQUIRED
)
message("CLGTF include dir: ${CGLTF_INCLUDE_DIR}")

add_executable(Crucible
  src/main.cc

  src/vulkan/render.cc
  src/vulkan/window.cc 
  src/vulkan/vk_context.cc
  src/vulkan/vk_pipeline.cc
  src/vulkan/vk_command.cc
  src/vulkan/vk_buffer.cc
  src/vulkan/vk_descriptor.cc
  src/vulkan/vma_impl.cc

)

target_include_directories(Crucible PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
    ${CGLTF_INCLUDE_DIR}
)

target_precompile_headers(Crucible PRIVATE
  "src/pch.h"
)

add_library(cgltf INTERFACE)

target_link_libraries(Crucible PRIVATE
  glfw
  VulkanMemoryAllocator
  ${Vulkan_LIBRARIES}
)

target_compile_definitions(Crucible PRIVATE
  SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/"
)


add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/shaders/vert.spv ${CMAKE_BINARY_DIR}/shaders/frag.spv
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
  COMMAND glslc -fshader-stage=vertex ${CMAKE_SOURCE_DIR}/src/shaders/vert.glsl -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
  COMMAND glslc -fshader-stage=fragment ${CMAKE_SOURCE_DIR}/src/shaders/frag.glsl -o ${CMAKE_BINARY_DIR}/shaders/frag.spv
  DEPENDS ${CMAKE_SOURCE_DIR}/src/shaders/vert.glsl ${CMAKE_SOURCE_DIR}/src/shaders/frag.glsl
)
add_custom_target(shaders DEPENDS ${CMAKE_BINARY_DIR}/shaders/vert.spv ${CMAKE_BINARY_DIR}/shaders/frag.spv)
add_dependencies(Crucible shaders)
