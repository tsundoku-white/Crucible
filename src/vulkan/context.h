#pragma once

#include "src/core/pch.h"
#include <cstdint>
#include <src/vulkan/window.h>
#include <vulkan/vulkan_core.h>
#include <vector>  // Add this

struct Context
{
  VkInstance                m_instance              = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT  m_debugMessenger        = VK_NULL_HANDLE;
  VkDevice                  m_device                = VK_NULL_HANDLE;
  VkPhysicalDevice          m_physicalDevice        = VK_NULL_HANDLE;
  VkQueue                   m_queue                 = VK_NULL_HANDLE; 
  VmaAllocator              m_allocator             = VK_NULL_HANDLE;
  VkSurfaceKHR              m_surface               = VK_NULL_HANDLE;
  VkSwapchainKHR            m_swapchain             = VK_NULL_HANDLE;
  VkExtent2D                m_swapchain_extent      = {0,0};
  VkFormat                  m_swapchainImageFormat  = VK_FORMAT_UNDEFINED;
  std::vector<VkImage>      m_swapchainImages       = {};
  std::vector<VkImageView>  m_swapchainImageViews   = {};
  VkFormat                  m_depthFormat           = VK_FORMAT_UNDEFINED;
  VkImage                   m_depthImage            = VK_NULL_HANDLE;
  VkImageView               m_depthImageView        = VK_NULL_HANDLE;
  VmaAllocation             m_depthAllocation       = VK_NULL_HANDLE;
  uint32_t                  m_queueFamily           = 0;

  VkImage                   m_msaaColorImage        = VK_NULL_HANDLE;
  VkImageView               m_msaaColorImageView    = VK_NULL_HANDLE;
  VmaAllocation             m_msaaColorAllocation   = VK_NULL_HANDLE;

  VkSampleCountFlagBits     m_msaaSamples           = VK_SAMPLE_COUNT_1_BIT;
  uint32_t                  m_msaa                  = 4;
  bool                      m_vsyncEnabled          = false;
};

namespace n_context
{
  void createContext(Context &context, Window &window);
  void destroyContext(Context &context); 
  void createSwapchain(Context &context, Window &window);
  void destroySwapchain(Context &context);
  void recreateSwapchain(Context &context, Window &window); 
  VkSampleCountFlagBits getMaxUsableSampleCount(Context &context);

}
