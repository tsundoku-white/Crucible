#pragma once

#include "src/core/pch.h"

struct Buffer;
struct Context;

struct Descriptor
{
  VkDescriptorSetLayout         m_layout    = VK_NULL_HANDLE;
  VkDescriptorPool              m_pool      = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet>  m_sets      = {};
  VkSampler                     m_sampler   = VK_NULL_HANDLE;
  VkImageView m_imageView;
};

namespace n_descriptor
{
  void createDescriptorSetLayout(Descriptor &descriptor, Context &context);
  void createDescriptorSets(Descriptor &descriptor, Context &context,
      Buffer &uboBuffer, Buffer &ssboBuffer, VkImageView &imageView ,uint32_t frameCount);
  void destoryDescriptor(Descriptor &descriptor, Context &context);
}
