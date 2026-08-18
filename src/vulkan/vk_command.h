#pragma once
#include "src/pch.h"
#include "src/vulkan/vk_descriptor.h"

struct Vk_Context;
struct Vk_Pipeline;
struct Vk_Buffer;

struct Vk_Command
{
  VkDevice m_device = VK_NULL_HANDLE;

  // primary buffer per frame-in-flight — this is what gets submitted
  std::vector<VkCommandPool>   m_primary_pools;
  std::vector<VkCommandBuffer> m_primary_buffers;

  // secondary buffers, indexed [frame][thread]
  std::vector<std::vector<VkCommandPool>>   m_thread_pools;
  std::vector<std::vector<VkCommandBuffer>> m_secondary_buffers;

  uint32_t m_queue_family_index = 0;
  uint32_t m_thread_count       = 0;
};

namespace n_commands
{
  void create(Vk_Command &cmd, Vk_Context &context, uint32_t frame_count, uint32_t thread_count);
  void destroy(Vk_Command &cmd);

  // records one thread's slice of the scene into its secondary buffer
  void record_secondary(Vk_Command &cmd, Vk_Context &context, Vk_Pipeline &pip,
      Vk_Buffer &vertex_buffer, Vk_Descriptor &descriptor,uint32_t frame_index, uint32_t thread_index,
      uint32_t first_vertex, uint32_t vertex_count);

  // stitches all secondaries for this frame into the primary and submits-ready state
  void record_primary(Vk_Command &cmd, Vk_Context &context, uint32_t frame_index, uint32_t image_index);
}
