#pragma once

#include "src/core/pch.h"

struct Context;
struct Command;

struct Texture
{
  VkImage        image      = VK_NULL_HANDLE;
  VmaAllocation  allocation = VK_NULL_HANDLE;
  VkImageView    view       = VK_NULL_HANDLE;
  uint32_t       width = 0, height = 0, channels = 4, mipLevels = 1;
};

namespace n_image
{
  void createTexture(Texture &texture, Context &context, Command &command, std::string path);
  VkImageView createImageView(Context &context, VkImage image, VkFormat format);
  void destroyTexture(Texture &texture, Context &context);
}
