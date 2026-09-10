#include "image.h"
#include <src/core/pch.h>
#include <src/vulkan/command.h>
#include <src/vulkan/context.h>
#include "src/vulkan/buffer.h"
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace n_image 
{
  VkImageView createImageView(Context &context, Texture &texture, VkFormat format)
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = format;

    viewInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel    = 0;
    viewInfo.subresourceRange.levelCount      = texture.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer  = 0;
    viewInfo.subresourceRange.layerCount      = 1;

    VkImageView imageView;
    vkCheck(vkCreateImageView(context.m_device, &viewInfo, nullptr, &imageView),
        "failed to create image view");

    return imageView;
  }

void generateMipmaps(Context &context, Command &command, Texture &texture,
    VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(context.m_physicalDevice, imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    throw std::runtime_error("texture image format does not support linear blitting");

  VkCommandBuffer commandBuffer = n_command::beginSingleTime(command, context);

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = texture.image;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  int32_t mipWidth  = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0]                 = {0, 0, 0};
    blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = {0, 0, 0};
    blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1,
                                           mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    vkCmdBlitImage(commandBuffer,
        texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    if (mipWidth > 1)  mipWidth  /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
      0, nullptr, 0, nullptr, 1, &barrier);

  n_command::endSingleTime(command, context, commandBuffer);
}

  void createTexture(Texture &texture, Context &context, Command &command, std::string path)
  {
    int width, height, channels;
    stbi_uc *pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
      throw std::runtime_error("failed to load texture image: " + path);

    texture.width  = static_cast<uint32_t>(width);
    texture.height = static_cast<uint32_t>(height);

    texture.mipLevels = static_cast<uint32_t>(
        std::floor(std::log2(std::max(width, height)))) + 1;

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texture.width) * texture.height * 4;

    Buffer staging{};
    n_buffer::create_buffer(staging, context, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    std::memcpy(staging.m_mapped, pixels, imageSize);
    stbi_image_free(pixels);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = texture.width;
    imageInfo.extent.height = texture.height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = texture.mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vkCheck(vmaCreateImage(context.m_allocator, &imageInfo, &allocInfo,
          &texture.image, &texture.allocation, nullptr),
        "failed to create texture image");

    n_buffer::transitionImageLayout(context, command, texture, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    n_buffer::copyBufferToImage(context, command, staging.m_buffer, texture.image, texture.width, texture.height);

    n_buffer::destroyBuffer(staging, context);
    n_image::generateMipmaps(context, command, texture, VK_FORMAT_R8G8B8A8_SRGB,
    texture.width, texture.height, texture.mipLevels);

    texture.view = createImageView(context, texture, VK_FORMAT_R8G8B8A8_SRGB);
  }
  void destroyTexture(Texture &texture, Context &context)
  {
    if (texture.image != VK_NULL_HANDLE)
      vkDestroyImage(context.m_device, texture.image, nullptr);

    if (texture.allocation != VK_NULL_HANDLE)
      vmaFreeMemory(context.m_allocator, texture.allocation);

    if (texture.view != VK_NULL_HANDLE)
      vkDestroyImageView(context.m_device, texture.view, nullptr);
  }
}
