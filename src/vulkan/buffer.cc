#include "buffer.h"

#include "src/vulkan/context.h"
#include "src/vulkan/command.h"
#include <vulkan/vulkan_core.h>

namespace n_buffer 
{
  void create_buffer(Buffer &buffer, Context &context, VkDeviceSize size,
      VkBufferUsageFlags usage, VmaMemoryUsage memory_usage,
      VmaAllocationCreateFlags alloc_flags)
  {
    if (size == 0)
      throw std::runtime_error("create_buffer: attempted to create a zero-size buffer");

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memory_usage;
    allocInfo.flags = alloc_flags;

    VmaAllocationInfo nAllocInfo{};
    if (vmaCreateBuffer(context.m_allocator, &bufferInfo, &allocInfo,
          &buffer.m_buffer, &buffer.m_allocation, &nAllocInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to create buffer");
    }

    buffer.m_size   = size;
    buffer.m_mapped = nAllocInfo.pMappedData;

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
      VkBufferDeviceAddressInfo addInfo{};
      addInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
      addInfo.buffer = buffer.m_buffer;
      buffer.m_address = vkGetBufferDeviceAddress(context.m_device, &addInfo);
    }
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

  void transitionImageLayout(Context &context, Command &command, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
  {
    VkCommandBuffer commandBuffer = n_command::beginSingleTime(command, context);


    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
      throw std::runtime_error("unsupported image layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    n_command::endSingleTime(command, context, commandBuffer);
  }

  
 void copyBufferToImage(Context &context, Command &command, VkBuffer buffer,
    VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer cmd = n_command::beginSingleTime(command, context);

  VkBufferImageCopy region{};
  region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
  region.imageExtent      = { width, height, 1 };

  vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  n_command::endSingleTime(command, context, cmd);
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

  void destroyBuffer(Buffer &buffer, Context &context)
  {
    if (buffer.m_buffer != VK_NULL_HANDLE)
      vmaDestroyBuffer(context.m_allocator, buffer.m_buffer, buffer.m_allocation);
  }

  void createUniformBuffer(Buffer &buffer, Context &context, VkDeviceSize size)
  {
    create_buffer(buffer, context, size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
  }

  void createStorageBuffer(Buffer &buffer, Context &context, VkDeviceSize size)
  {
    create_buffer(buffer, context, size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
  }

  void updateBuffer(Buffer &buffer, const void* data, VkDeviceSize size)
  {
    std::memcpy(buffer.m_mapped, data, size);
  }

}
