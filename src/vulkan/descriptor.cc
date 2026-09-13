#include "descriptor.h"
#include "src/vulkan/buffer.h"
#include "src/core/i_resource.h"
#include <array>
#include <cstdint>
#include <src/core/pch.h>
#include <vector>
#include "src/vulkan/image.h"

constexpr uint32_t MAX_TEXTURES = 12;

namespace n_descriptor
{
  void createSampler(Descriptor &descriptor, Context &context) 
  {
    VkSamplerCreateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter          = VK_FILTER_LINEAR;
    info.minFilter          = VK_FILTER_LINEAR;

    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    info.anisotropyEnable   = VK_TRUE;
    info.maxAnisotropy      = 16.0f;
    info.borderColor        = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates  = VK_FALSE;
    info.mipmapMode               = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.mipLodBias               = -0.75f;
    info.maxLod                   = VK_LOD_CLAMP_NONE;
    vkCheck(vkCreateSampler(context.m_device, &info, nullptr, &descriptor.m_sampler), 
        "failed to create sampler");
  }

   void createDescriptorSetLayout(Descriptor &descriptor, Context &context)
   {
        // ---- UBO (binding 0) ----
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = 0;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    // ---- SSBO (binding 1) ----
    VkDescriptorSetLayoutBinding ssboLayoutBinding{};
    ssboLayoutBinding.binding         = 1;
    ssboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboLayoutBinding.descriptorCount = 1;
    ssboLayoutBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding  samplerLayoutBinding{};
    samplerLayoutBinding.binding            = 2;
    samplerLayoutBinding.descriptorCount    = MAX_TEXTURES;
    samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
      uboLayoutBinding,
      ssboLayoutBinding, 
      samplerLayoutBinding
    };

    std::array<VkDescriptorBindingFlags, 3> bindingFlags = {
      0, 0,
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
    bindingFlagsInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.bindingCount  = static_cast<uint32_t>(bindingFlags.size());
    bindingFlagsInfo.pBindingFlags = bindingFlags.data();


    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext        = &bindingFlagsInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(context.m_device, &layoutInfo, nullptr, &descriptor.m_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }
   }

  void createDescriptorSets(Descriptor &descriptor, Context &context,
      Buffer &uboBuffer, Buffer &ssboBuffer, std::vector<Texture> textures ,uint32_t frameCount)
  {

    createSampler(descriptor, context);
    std::array<VkDescriptorPoolSize, 3> poolSize;
    poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize[0].descriptorCount = frameCount;
    poolSize[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize[1].descriptorCount = frameCount;
    poolSize[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize[2].descriptorCount = frameCount * MAX_TEXTURES;

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

    uint32_t textureCount = static_cast<uint32_t>(textures.size());
    std::vector<uint32_t> variableCounts(frameCount, textureCount);

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
    variableCountInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableCountInfo.descriptorSetCount = frameCount;
    variableCountInfo.pDescriptorCounts  = variableCounts.data();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext              = &variableCountInfo;
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

    std::vector<VkDescriptorImageInfo> imageInfos(textureCount);
    for (uint32_t i = 0; i < textureCount; ++i) {
      imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfos[i].imageView   = textures[i].m_view;
      imageInfos[i].sampler     = descriptor.m_sampler;
    }

    for (uint32_t i = 0; i < frameCount; ++i)
    {
      std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

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

      // ---- Sampler Write ----
      descriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[2].dstSet          = descriptor.m_sets[i];
      descriptorWrites[2].dstBinding      = 2;
      descriptorWrites[2].dstArrayElement = 0;
      descriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[2].descriptorCount = textureCount;
      descriptorWrites[2].pImageInfo      = imageInfos.data();

      vkUpdateDescriptorSets(context.m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

  }

  void destoryDescriptor(Descriptor &descriptor, Context &context)
  {
    if (descriptor.m_sampler != VK_NULL_HANDLE)
      vkDestroySampler(context.m_device, descriptor.m_sampler, nullptr);

    if (descriptor.m_pool != VK_NULL_HANDLE)
      vkDestroyDescriptorPool(context.m_device, descriptor.m_pool, nullptr);

    if (!descriptor.m_sets.empty())
      descriptor.m_sets.clear();

    if (descriptor.m_layout != VK_NULL_HANDLE)
      vkDestroyDescriptorSetLayout(context.m_device, descriptor.m_layout, nullptr);
  }
}
