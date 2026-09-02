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
};

namespace n_context
{
  void createContext(Context &context, Window &window);
  void destoryContext(Context &context);
}
