#pragma once
#include "src/pch.h"

//forward decloration
struct Vk_Window;

struct Vk_Context
{
  VkInstance                m_instance;
  VkDebugUtilsMessengerEXT  m_debug_messenger;
  VkPhysicalDevice          m_physical_device;
  VkDevice                  m_device;
  VkSurfaceKHR              m_surface;
  VmaAllocator              m_allocator;
 
  VkQueue                   m_graphics_queue;
  VkQueue                   m_present_queue;
 
  VkSwapchainKHR            m_swapchain;
  std::vector<VkImage>      m_swapchain_images;
  VkFormat                  m_swapchain_image_format;
  VkExtent2D                m_swapchain_extent;
  std::vector<VkImageView>  m_swapchain_image_views;

  bool m_vsync;
};
 
namespace n_context
{
  void create(Vk_Context &context, Vk_Window &vk_window);
  void create_allocator(Vk_Context &vk_context);
  void create_swapchain(Vk_Context &vk_context, Vk_Window &vk_window);
  void create_image_views(Vk_Context &vk_context);
  void destroy_image_views(Vk_Context &vk_context);
  void destroy_swapchain(Vk_Context &vk_context);
  void destroy(Vk_Context &context);
}
 

