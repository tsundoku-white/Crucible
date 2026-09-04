#pragma once

#include "src/core/pch.h"
#include <vulkan/vulkan_core.h>

struct Buffer;
struct Context;

struct Descriptor
{
  VkDescriptorSetLayout m_layout  = VK_NULL_HANDLE;
  VkDescriptorPool      m_pool    = VK_NULL_HANDLE;
  VkDescriptorSet       m_set     = VK_NULL_HANDLE;
};

namespace n_descriptor
{
  void createDescriptor(Descriptor &descriptor, Context &context, Buffer &uboBuffer, Buffer &ssboBuffer);
  void destoryDescriptor(Descriptor &descriptor, Context &context);
}
