#pragma once

#include <cstdlib>
#include <vulkan/vulkan_core.h>
#ifndef NDEBUG
static constexpr bool g_debug = true;
#else
static constexpr bool g_debug = false;
#endif

// vulkan include
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

// glfw include
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// math
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Common Includes
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <algorithm>

// memory management
#include <memory>

// Debug
#include <print>
#include <stdexcept>
#include <format>

inline void printProgressBar(int current, int total) {
  int bar_width = 25; 
  float progress = static_cast<float>(current) / total;
  if (progress > 1.0f) progress = 1.0f;                              
  int filled_length = static_cast<int>(bar_width * progress + 0.5f);
  std::print("\r["); 
  for (int i = 0; i < bar_width; ++i) {
    if (i < filled_length) std::print("#");
    else std::print("_");
  }
  std::print("] {:.1f}%", progress * 100.0f);
  std::fflush(stdout);

  if (current + 1 == total)
    std::print("\n");
}
