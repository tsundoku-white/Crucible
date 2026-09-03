#pragma once

#include "src/core/pch.h"
#include <src/vulkan/window.h>
#include <vulkan/vulkan_core.h>

struct Context
{
  VkInstance                m_instance        = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT  m_debugMessenger  = VK_NULL_HANDLE;
  VkDevice                  m_device          = VK_NULL_HANDLE;
  VkPhysicalDevice          m_physicalDevice  = VK_NULL_HANDLE;
  VkQueue                   m_queue           = VK_NULL_HANDLE; 
  VmaAllocator              m_allocator       = VK_NULL_HANDLE;
  VkSurfaceKHR              m_surface         = VK_NULL_HANDLE;
  VkSwapchainKHR            m_swapchain       = VK_NULL_HANDLE;
  VkExtent2D                m_swapchain_extent = {0,0};
  VkFormat                  m_swapchainImageFormat = VK_FORMAT_UNDEFINED;

  std::vector<VkImage>      m_swapchainImages;
  std::vector<VkImageView>  m_swapchainImageViews;

  VkFormat                  m_depthFormat     = VK_FORMAT_UNDEFINED;
  VkImage                   m_depthImage      = VK_NULL_HANDLE;
  VkImageView               m_depthImageView  = VK_NULL_HANDLE;
  VmaAllocation             m_depthAllocation = VK_NULL_HANDLE;
  uint32_t                  m_queueFamily     = 0;
};

namespace n_context
{
  void createContext(Context &context, Window &window);
  void destoryContext(Context &context);
}
