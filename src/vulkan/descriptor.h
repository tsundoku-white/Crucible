#pragma once

#include "src/core/pch.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <cstdint>

struct Buffer;
struct Context;

struct Descriptor
{
  VkDescriptorSetLayout        m_layout = VK_NULL_HANDLE;
  VkDescriptorPool              m_pool   = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet>  m_sets   = {};   // one per frame-in-flight
};

namespace n_descriptor
{
  void createDescriptor(Descriptor &descriptor, Context &context, Buffer &uboBuffer, Buffer &ssboBuffer, uint32_t frameCount);
  void destoryDescriptor(Descriptor &descriptor, Context &context);
}
