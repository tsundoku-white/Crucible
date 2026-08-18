#include "vk_descriptor.h"
#include "src/vulkan/vk_context.h"

namespace n_descriptor
{
  void create(Vk_Descriptor &descriptor, Vk_Context &context, uint32_t frame_count)
  {
    // --- descriptor set layout: binding 0 = ShaderData UBO, vertex stage ---
    VkDescriptorSetLayoutBinding ubo_binding{};
    ubo_binding.binding         = 0;
    ubo_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings    = &ubo_binding;

    if (vkCreateDescriptorSetLayout(context.m_device, &layout_info, nullptr, &descriptor.m_layout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }

    // --- UBO buffers, one per frame-in-flight ---
    descriptor.m_ubo_buffers.resize(frame_count);
    for (uint32_t i = 0; i < frame_count; i++) {
      n_buffer::create_buffer(context, sizeof(ShaderData),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VMA_MEMORY_USAGE_AUTO,
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
          descriptor.m_ubo_buffers[i]);
    }

    // --- descriptor pool ---
    VkDescriptorPoolSize pool_size{};
    pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = frame_count;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes    = &pool_size;
    pool_info.maxSets       = frame_count;

    if (vkCreateDescriptorPool(context.m_device, &pool_info, nullptr, &descriptor.m_pool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor pool!");
    }

    // --- descriptor sets, one per frame-in-flight ---
    std::vector<VkDescriptorSetLayout> layouts(frame_count, descriptor.m_layout);

    VkDescriptorSetAllocateInfo set_alloc_info{};
    set_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_alloc_info.descriptorPool     = descriptor.m_pool;
    set_alloc_info.descriptorSetCount = frame_count;
    set_alloc_info.pSetLayouts        = layouts.data();

    descriptor.m_sets.resize(frame_count);
    if (vkAllocateDescriptorSets(context.m_device, &set_alloc_info, descriptor.m_sets.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (uint32_t i = 0; i < frame_count; i++) {
      VkDescriptorBufferInfo buffer_info{};
      buffer_info.buffer = descriptor.m_ubo_buffers[i].m_buffer;
      buffer_info.offset = 0;
      buffer_info.range  = sizeof(ShaderData);

      VkWriteDescriptorSet write{};
      write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet          = descriptor.m_sets[i];
      write.dstBinding      = 0;
      write.dstArrayElement = 0;
      write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      write.descriptorCount = 1;
      write.pBufferInfo     = &buffer_info;

      vkUpdateDescriptorSets(context.m_device, 1, &write, 0, nullptr);
    }

    std::print("\e[0;32m" "pass: " "\e[0m" "vk descriptor\n");
  }

  void destroy(Vk_Descriptor &descriptor, Vk_Context &context)
  {
    for (auto &buf : descriptor.m_ubo_buffers)
      n_buffer::destroy_buffer(context, buf);
    descriptor.m_ubo_buffers.clear();

    if (descriptor.m_pool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(context.m_device, descriptor.m_pool, nullptr);
      descriptor.m_pool = VK_NULL_HANDLE;
    }
    if (descriptor.m_layout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(context.m_device, descriptor.m_layout, nullptr);
      descriptor.m_layout = VK_NULL_HANDLE;
    }
  }

  void update_ubo(Vk_Descriptor &descriptor, uint32_t frame_index, const ShaderData &data)
  {
    std::memcpy(descriptor.m_ubo_buffers[frame_index].m_mapped, &data, sizeof(ShaderData));
  }
}
