#include "buffer.h"

#include "src/vulkan/context.h"
#include "src/vulkan/command.h"

namespace n_buffer 
{
  void create_buffer(Buffer &buffer, Context &context, VkDeviceSize size,
      VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
      VmaAllocationCreateFlags alloc_flags)
  {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memory_usage;
    allocInfo.flags = alloc_flags;

    VkBufferDeviceAddressInfo addInfo{};
    addInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addInfo.buffer = buffer.m_buffer;

    VkDeviceAddress address = vkGetBufferDeviceAddress(context.m_device, &addInfo);

    // btw n in name means new
    VmaAllocationInfo nAllocInfo{};
    vmaGetAllocationInfo(context.m_allocator, buffer.m_allocation, &nAllocInfo);

    if (vmaCreateBuffer(context.m_allocator, &bufferInfo, &allocInfo,
          &buffer.m_buffer, &buffer.m_allocation, &nAllocInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to create buffer");
    }

    buffer.m_size   = size;
    buffer.m_mapped = nAllocInfo.pMappedData;
  }

  static void copyBuffer(Context &context, Command &command, VkBuffer src, VkBuffer dst, VkDeviceSize size)
  {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = command.m_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.m_device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    vkQueueSubmit(context.m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.m_queue); 

    vkFreeCommandBuffers(context.m_device, command.m_pool, 1, &cmd);
  }


  void createVertexBuffer(Buffer &buffer, Command &command, Context &context, std::vector<Vertex> vertices)
  {
    VkDeviceSize size = sizeof(Vertex) * vertices.size();

    Buffer staging{};
    create_buffer(staging, context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    std::memcpy(staging.m_mapped, vertices.data(), size);

    create_buffer(buffer, context, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO, 0);

    copyBuffer(context, command, staging.m_buffer, buffer.m_buffer, size);
    destroyBuffer(staging, context);
  }

  void createIndexBuffer(Buffer &buffer, Command &command, Context &context, std::vector<uint32_t> indices)
  {
    VkDeviceSize size = sizeof(uint32_t) * indices.size();

    Buffer staging{};
    create_buffer(staging, context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    std::memcpy(staging.m_mapped, indices.data(), size);

    create_buffer(buffer, context, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO, 0);

    copyBuffer(context, command, staging.m_buffer, buffer.m_buffer, size);
    destroyBuffer(staging, context);
  }

  void destoryBuffer(Buffer &buffer, Context &context)
  {
    if (buffer.m_buffer == VK_NULL_HANDLE) return;
    vmaDestroyBuffer(context.m_allocator, buffer.m_buffer, buffer.m_allocation);
    buffer.m_buffer     = VK_NULL_HANDLE;
    buffer.m_allocation = VK_NULL_HANDLE;
    buffer.m_mapped     = nullptr;
  }

}
