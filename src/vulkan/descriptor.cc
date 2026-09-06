#include "descriptor.h"
#include "src/vulkan/buffer.h"
#include "src/core/i_resource.h"
#include <array>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace n_descriptor
{
  void createDescriptorSets(Descriptor &descriptor, Context &context, VkDescriptorSetLayout layout,
      Buffer &uboBuffer, Buffer &ssboBuffer, uint32_t frameCount)
  {
    descriptor.m_layout = layout;

    std::array<VkDescriptorPoolSize, 2> poolSize;
    poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = frameCount;
    poolSize[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize[1].descriptorCount = frameCount;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount  = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes     = poolSize.data();
    poolInfo.maxSets        = frameCount;

    if (vkCreateDescriptorPool(context.m_device, &poolInfo, nullptr, &descriptor.m_pool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor pool!");
    }

    std::vector<VkDescriptorSetLayout> layouts(frameCount, descriptor.m_layout);
    descriptor.m_sets.resize(frameCount);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptor.m_pool;
    allocInfo.descriptorSetCount = frameCount;
    allocInfo.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(context.m_device, &allocInfo, descriptor.m_sets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }

    VkDescriptorBufferInfo uboBufferInfo{};
    uboBufferInfo.buffer  = uboBuffer.m_buffer;
    uboBufferInfo.offset  = 0;
    uboBufferInfo.range   = sizeof(UniformBufferObject);

    VkDescriptorBufferInfo ssboBufferInfo{};
    ssboBufferInfo.buffer = ssboBuffer.m_buffer;
    ssboBufferInfo.offset = 0;
    ssboBufferInfo.range  = sizeof(ShaderStorageBufferObject);

    for (uint32_t i = 0; i < frameCount; ++i)
    {
      std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

      // ---- UBO Write ----
      descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet          = descriptor.m_sets[i];
      descriptorWrites[0].dstBinding      = 0;
      descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo     = &uboBufferInfo;

      // ---- SSBO Write ----
      descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet          = descriptor.m_sets[i];
      descriptorWrites[1].dstBinding      = 1;
      descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo     = &ssboBufferInfo;

      vkUpdateDescriptorSets(context.m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
  }

  void destoryDescriptor(Descriptor &descriptor, Context &context)
  {
    if (descriptor.m_pool != VK_NULL_HANDLE)
      vkDestroyDescriptorPool(context.m_device, descriptor.m_pool, nullptr);

    if (!descriptor.m_sets.empty())
      descriptor.m_sets.clear();

    descriptor.m_layout = VK_NULL_HANDLE;
    descriptor.m_pool   = VK_NULL_HANDLE;
  }
}
