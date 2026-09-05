#pragma once

#include "src/core/pch.h"
#include <vulkan/vulkan_core.h>

struct Context;
struct Command;

struct alignas(16) UniformBufferObject
{
  // camera projection
  glm::mat4 m_projectionMatrix;
  glm::mat4 m_viewMatrix;
};

struct alignas(16) ShaderStorageBufferObject
{
  // models
  glm::mat4 m_modelsMatrix = glm::mat4(1.f);
  size_t    m_modelCount   = 0;

  // textures
  int32_t   m_indices       = 0;
  size_t    m_indicesCount = 0;
};

struct Vertex {
  glm::vec3 m_pos;
  glm::vec3 m_normal;
  glm::vec2 m_uv;

    static VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description {
      .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    return binding_description;
  }

  static std::array<VkVertexInputAttributeDescription, 3> get_attributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, m_pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, m_normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, m_uv);

    return attributeDescriptions;
  }
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
  void create_buffer(Buffer &buffer, Context &context, VkDeviceSize size,
      VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
      VmaAllocationCreateFlags alloc_flags);

  static void copyBuffer(Context &context, Command &command, VkBuffer src, VkBuffer dst, VkDeviceSize size);

  void createVertexBuffer(Buffer &buffer, Command &command, Context &context, std::vector<Vertex> vertices);

  void createIndexBuffer(Buffer &buffer, Command &command, Context &context, std::vector<uint32_t> indices);

   void destroyBuffer(Buffer &buffer, Context &context);

   void createUniformBuffer(Buffer &buffer, Context &context, VkDeviceSize size);
   void createStorageBuffer(Buffer &buffer, Context &context, VkDeviceSize size);
   void updateBuffer(Buffer &buffer, const void* data, VkDeviceSize size);
}
