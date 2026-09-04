#pragma once

#include "src/core/pch.h"
#include <vulkan/vulkan_core.h>

struct Context;
struct Command;

struct alignas(16) UniformBufferObject
{
  // camera projection
  glm::mat4 m_projection_matrix;
  glm::mat4 m_view_matrix;
};

struct alignas(16) ShaderStorageBufferObject
{
  // models
  glm::mat4 m_models_matrix = glm::mat4(1.f);
  size_t    m_model_count   = 0;

  // textures
  int32_t   m_indices       = 0;
  size_t    m_indices_count = 0;
};

struct Vertex {
  glm::vec3 m_pos;
  glm::vec3 m_normal;
  glm::vec2 m_uv;
};

struct Buffer
{
  VkBuffer          m_buffer      = VK_NULL_HANDLE;
  VmaAllocation     m_allocation  = VK_NULL_HANDLE;
  VmaAllocationInfo m_info        = {};
  VkDeviceSize      m_size        = 0;
  void*             m_mapped      = nullptr;
  VkDeviceAddress   m_address     = 0;
};

namespace n_buffer
{
  void createVertexBuffer(Buffer &buffer, Command &command, Context &context, std::vector<Vertex> vertices);
  void createIndexBuffer(Buffer &buffer, Command &command, Context &context, std::vector<uint32_t> indices);
  void destroyBuffer(Buffer &buffer, Context &context);
}
