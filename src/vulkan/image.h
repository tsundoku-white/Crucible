#pragma once

#include "src/core/pch.h"

struct Context;
struct Command;

struct Texture
{
  VkImage        m_image      = VK_NULL_HANDLE;
  VmaAllocation  m_allocation = VK_NULL_HANDLE;
  VkImageView    m_view       = VK_NULL_HANDLE;
  uint32_t       m_width = 0, m_height = 0, m_channels = 4, m_mipLevels = 1;
};

namespace n_image
{
  void        createTexture(Texture &texture, Context &context, Command &command, std::string path);
  VkImageView createImageView(Context &context, VkImage image, VkFormat format);
  void        destroyTexture(Texture &texture, Context &context);
  void        generateMipmaps(Context &context, Command &command, Texture &texture,
      VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

}
