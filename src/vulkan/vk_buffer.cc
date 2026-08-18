#include "vk_buffer.h"
#include "src/vulkan/vk_context.h"
#include "src/vulkan/vk_command.h"

namespace n_buffer
{
  void create_buffer(Vk_Context &context, VkDeviceSize size, VkBufferUsageFlags usage,
                      VmaMemoryUsage memory_usage, VmaAllocationCreateFlags alloc_flags,
                      Vk_Buffer &out_buffer)
  {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size        = size;
    buffer_info.usage       = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;
    alloc_info.flags = alloc_flags;

    VmaAllocationInfo out_alloc_info{};

    if (vmaCreateBuffer(context.m_allocator, &buffer_info, &alloc_info,
          &out_buffer.m_buffer, &out_buffer.m_allocation, &out_alloc_info) != VK_SUCCESS) {
      throw std::runtime_error("failed to create buffer");
    }

    out_buffer.m_size   = size;
    out_buffer.m_mapped = out_alloc_info.pMappedData; 
  }

  void destroy_buffer(Vk_Context &context, Vk_Buffer &buffer)
  {
    if (buffer.m_buffer == VK_NULL_HANDLE) return;
    vmaDestroyBuffer(context.m_allocator, buffer.m_buffer, buffer.m_allocation);
    buffer.m_buffer     = VK_NULL_HANDLE;
    buffer.m_allocation = VK_NULL_HANDLE;
    buffer.m_mapped     = nullptr;
  }

  static void copy_buffer(Vk_Context &context, Vk_Command &command, VkBuffer src, VkBuffer dst, VkDeviceSize size)
  {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool        = command.m_primary_pools[0];
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.m_device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copy_region);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{};
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &cmd;

    vkQueueSubmit(context.m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.m_graphics_queue); // fine for a one-off setup copy

    vkFreeCommandBuffers(context.m_device, command.m_primary_pools[0], 1, &cmd);
  }

  void create_vertex_buffer(Vk_Context &context, Vk_Command &command, Vk_Buffer &out_buffer)
  {
static const std::vector<Vertex> vertices = {
    { {  0.0f,  0.5f,  0.0f }, {0,0,1}, {0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} },
    { {  0.5f, -0.5f,  0.0f }, {0,0,1}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
    { { -0.5f, -0.5f,  0.0f }, {0,0,1}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} },
  };
  VkDeviceSize size = sizeof(Vertex) * vertices.size();

    Vk_Buffer staging{};
    create_buffer(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        staging);

    std::memcpy(staging.m_mapped, vertices.data(), size);

    create_buffer(context, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO, 0, out_buffer);

    copy_buffer(context, command, staging.m_buffer, out_buffer.m_buffer, size);
    destroy_buffer(context, staging);

    std::print("\e[0;32m" "pass: " "\e[0m" "vk buffer\n");
  }
}
