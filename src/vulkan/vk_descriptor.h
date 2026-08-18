#pragma once
#include "src/pch.h"
#include "src/vulkan/vk_buffer.h"
#include "src/vulkan/vk_context.h"
#include <cstdint>

struct Vk_Buffer;

struct Vk_Descriptor 
{
  VkDescriptorSetLayout        m_layout = VK_NULL_HANDLE;
  VkDescriptorPool             m_pool   = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_sets;         
  std::vector<Vk_Buffer>       m_ubo_buffers;
};

namespace n_descriptor
{
  void create(Vk_Descriptor &descriptor, Vk_Context &context, uint32_t frame_count);
  void destroy(Vk_Descriptor &descriptor, Vk_Context &context);
  void update_ubo(Vk_Descriptor &descriptor, uint32_t frame_index, const ShaderData &data);

}
