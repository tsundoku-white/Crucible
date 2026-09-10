#include "image.h"
#include <src/core/pch.h>
#include <src/vulkan/command.h>
#include <src/vulkan/context.h>
#include "src/vulkan/buffer.h"
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace n_image 
{
  VkImageView createImageView(Context &context, VkImage image, VkFormat format)
  {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image    = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format   = format;

    viewInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel    = 0;
    viewInfo.subresourceRange.levelCount      = 1;
    viewInfo.subresourceRange.baseArrayLayer  = 0;
    viewInfo.subresourceRange.layerCount      = 1;

    VkImageView imageView;
    vkCheck(vkCreateImageView(context.m_device, &viewInfo, nullptr, &imageView),
        "failed to create image view");

    return imageView;
  }

  void createTexture(Texture &texture, Context &context, Command &command, std::string path)
  {
    int width, height, channels;
    stbi_uc *pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
      throw std::runtime_error("failed to load texture image: " + path);

    texture.width  = static_cast<uint32_t>(width);
    texture.height = static_cast<uint32_t>(height);

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
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vkCheck(vmaCreateImage(context.m_allocator, &imageInfo, &allocInfo,
          &texture.image, &texture.allocation, nullptr),
        "failed to create texture image");

    n_buffer::transitionImageLayout(context, command, texture.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    n_buffer::copyBufferToImage(context, command, staging.m_buffer, texture.image, texture.width, texture.height);

    n_buffer::transitionImageLayout(context, command, texture.image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    n_buffer::destroyBuffer(staging, context);

  texture.view = createImageView(context, texture.image, VK_FORMAT_R8G8B8A8_SRGB);

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
