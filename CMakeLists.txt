cmake_minimum_required(VERSION 3.28)
project(Crucible CXX)

# windows is just behind so they use cxx23
if(MSVC)
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
  ## trageted cxx26
  set(CMAKE_CXX_STANDARD 26)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O")

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/dep_pkgs" CACHE PATH "")

set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL            OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_SAMPLES       OFF CACHE BOOL "" FORCE)

find_package(Vulkan REQUIRED COMPONENTS glslc glslangValidator)

## getting pkgs with cmake, using partial-clone + sparse-checkout so
## unneeded paths (docs/tests/examples/.git history) are never fetched
## instead of being cloned then deleted.
include(FetchContent)

FetchContent_Declare(
  VulkanMemoryAllocator
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator
    COMMAND git clone --filter=blob:none --no-checkout --depth 1
            --branch v3.2.1
            https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
            ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator sparse-checkout init --cone
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator sparse-checkout set
            include src cmake
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/VulkanMemoryAllocator checkout v3.2.1
)

FetchContent_Declare(
  CGLTF
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/cgltf
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${FETCHCONTENT_BASE_DIR}/cgltf
    COMMAND git clone --filter=blob:none --no-checkout --depth 1
            --branch v1.15 https://github.com/jkuhlmann/cgltf.git
            ${FETCHCONTENT_BASE_DIR}/cgltf
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/cgltf sparse-checkout init --cone
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/cgltf checkout v1.15
)

FetchContent_Declare(
  GLM
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/glm
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${FETCHCONTENT_BASE_DIR}/glm
    COMMAND git clone --filter=blob:none --no-checkout --depth 1
            --branch 1.0.3 https://github.com/g-truc/glm.git
            ${FETCHCONTENT_BASE_DIR}/glm
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glm sparse-checkout init --cone
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glm sparse-checkout set
            glm cmake
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glm checkout 1.0.3
)

FetchContent_Declare(
  GLFW
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/glfw
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${FETCHCONTENT_BASE_DIR}/glfw
    COMMAND git clone --filter=blob:none --no-checkout --depth 1
            --branch 3.5.1 https://github.com/glfw/glfw.git
            ${FETCHCONTENT_BASE_DIR}/glfw
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glfw sparse-checkout init --cone
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glfw sparse-checkout set
            include src CMake deps
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glfw checkout 3.5.1
)

FetchContent_Declare(
  STB
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/stb
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${FETCHCONTENT_BASE_DIR}/stb
    COMMAND git clone --filter=blob:none --no-checkout --depth 1
            https://github.com/nothings/stb.git
            ${FETCHCONTENT_BASE_DIR}/stb
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/stb sparse-checkout init --cone
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/stb checkout master
)

FetchContent_MakeAvailable(VulkanMemoryAllocator CGLTF GLM GLFW STB)

add_executable(Crucible
  ## main
  #src/main.cc

  bench/stress_test.cc
  ## core 
  src/core/i_render.cc
  src/core/i_resource.cc
  src/core/i_input.cc

  ## resources
  src/resources/load_mesh.cc

  ## vulkan
  src/vulkan/image.cc
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
  SHADER_DIR="${CMAKE_BINARY_DIR}/assets/shaders/"
  MODEL_PATH="${CMAKE_BINARY_DIR}/assets/models/"
  TEXTURE_PATH="${CMAKE_BINARY_DIR}/assets/textures/"
)

## asset copy
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

## shader compile
find_program(GLSLC_BIN glslc
  HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
  REQUIRED
)

set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
set(SHADER_BIN_DIR "${CMAKE_BINARY_DIR}/assets/shaders")

function(add_shader stage name)
  set(src "${SHADER_SRC_DIR}/${name}.glsl")
  set(out "${SHADER_BIN_DIR}/${name}.spv")

  add_custom_command(
    OUTPUT  ${out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BIN_DIR}
    COMMAND ${GLSLC_BIN} -fshader-stage=${stage} ${src} -o ${out}
    DEPENDS ${src}
    COMMENT "glslc: ${name}.glsl -> ${name}.spv"
    VERBATIM
  )
  set(SHADER_OUTPUTS ${SHADER_OUTPUTS} ${out} PARENT_SCOPE)
endfunction()

add_shader(vertex   shader_vert)
add_shader(fragment shader_frag)

add_custom_target(shaders DEPENDS ${SHADER_OUTPUTS})

## dep
add_dependencies(Crucible shaders)
add_dependencies(Crucible assets)

## auto-clean FetchContent scratch dirs after a successful build
add_custom_command(TARGET Crucible POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm -rf
    ${FETCHCONTENT_BASE_DIR}/glm-build
    ${FETCHCONTENT_BASE_DIR}/glm-subbuild
    ${FETCHCONTENT_BASE_DIR}/glfw-build
    ${FETCHCONTENT_BASE_DIR}/glfw-subbuild
    ${FETCHCONTENT_BASE_DIR}/cgltf-build
    ${FETCHCONTENT_BASE_DIR}/cgltf-subbuild
    ${FETCHCONTENT_BASE_DIR}/stb-build
    ${FETCHCONTENT_BASE_DIR}/stb-subbuild
    ${FETCHCONTENT_BASE_DIR}/vulkanmemoryallocator-build
    ${FETCHCONTENT_BASE_DIR}/vulkanmemoryallocator-subbuild
  COMMENT "Cleaning FetchContent..."
  VERBATIM
)
