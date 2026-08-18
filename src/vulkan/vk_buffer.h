#pragma once
#include "src/pch.h"
#include <cstddef>

// shader data 
struct ShaderData
{
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[3];
  glm::vec4 light {0,-10,10,0};
};

struct Vertex 
{
  glm::vec3 loc;
  glm::vec3 normal;
  glm::vec2 uv;

  glm::vec3 color;
  static VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description {
      .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    return binding_description;
  }

  static std::array<VkVertexInputAttributeDescription, 4> get_attribute_descriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attribute_descriptions{};

    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, loc);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, normal);

    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, uv);

    attribute_descriptions[3].binding = 0;
    attribute_descriptions[3].location = 3;
    attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[3].offset = offsetof(Vertex, color);

    return attribute_descriptions;
  }
};

// forward decloration
struct Vk_Context;
struct Vk_Command;

struct Vk_Buffer
{
  VkBuffer      m_buffer     = VK_NULL_HANDLE;
  VmaAllocation m_allocation = VK_NULL_HANDLE;
  VkDeviceSize  m_size       = 0;
  void*         m_mapped     = nullptr;
};

namespace n_buffer
{
  void create_buffer(Vk_Context &context, VkDeviceSize size, VkBufferUsageFlags usage,
                      VmaMemoryUsage memory_usage, VmaAllocationCreateFlags alloc_flags,
                      Vk_Buffer &out_buffer);

  void destroy_buffer(Vk_Context &context, Vk_Buffer &buffer);
  void create_vertex_buffer(Vk_Context &context, Vk_Command &command, Vk_Buffer &out_buffer);
}
