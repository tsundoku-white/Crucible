#include "descriptor.h"
#include "src/vulkan/buffer.h"
#include "src/vulkan/context.h"
#include <array>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

// i need commnet to keep track with what i happneing.
// thats why its like this ---- easier to seach
namespace n_descriptor
{
  void createDescriptor(Descriptor &descriptor, Context &context, Buffer &uboBuffer, Buffer &ssboBuffer, uint32_t frameCount)
  {
    // ---- UBO ----
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding          = 0;
    uboLayoutBinding.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount  = 1;
    uboLayoutBinding.stageFlags       = VK_SHADER_STAGE_VERTEX_BIT;

    // ---- SSBO ----
    VkDescriptorSetLayoutBinding ssboLayoutBinding{};
    ssboLayoutBinding.binding         = 1;
    ssboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboLayoutBinding.descriptorCount = 1;
    ssboLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, ssboLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(context.m_device, &layoutInfo, nullptr, &descriptor.m_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }

    // ---- Pool (sized for frameCount sets, 1 UBO + 1 SSBO each) ----
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

    // ---- Allocate (one set per frame-in-flight, all same layout) ----
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

    // ---- Buffer infos + writes ----
    // NOTE: currently every frame's set points at the SAME uboBuffer/ssboBuffer.
    // That's fine if the UBO/SSBO are themselves single shared buffers you update
    // per-frame before use. If you later make uboBuffer/ssboBuffer per-frame too
    // (an array/vector of Buffer, like Command::m_buffers), pass that in here and
    // index it per-frame inside this loop instead of using the same buffer for all.
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

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding    = 0;
    bindingDescription.stride     = sizeof(Vertex);
    bindingDescription.inputRate  = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    // ---- Position ----
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, m_pos);

    // ---- UV ----
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Vertex, m_uv);

    // ---- Normal ----
    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(Vertex, m_normal);
  }

  void destoryDescriptor(Descriptor &descriptor, Context &context)
  {
    // Destroying the pool also implicitly frees any sets allocated from it.
    vkDestroyDescriptorPool(context.m_device, descriptor.m_pool, nullptr);
    vkDestroyDescriptorSetLayout(context.m_device, descriptor.m_layout, nullptr);

    descriptor.m_pool   = VK_NULL_HANDLE;
    descriptor.m_layout = VK_NULL_HANDLE;
    descriptor.m_sets.clear();
  }
}
