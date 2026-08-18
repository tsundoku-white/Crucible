#include "render.h"
#include "src/vulkan/vk_descriptor.h"
#include "src/vulkan/window.h"
#include "src/vulkan/vk_context.h"
#include "src/vulkan/vk_pipeline.h"
#include "src/vulkan/vk_command.h"
#include "src/vulkan/vk_buffer.h"
#include <chrono>
#include <ratio>
#include <thread>

bool IRenderer::should_close() 
{ 
  return n_window::should_close(m_window); 
}

void IRenderer::create_sync_objects()
{
  auto m_last_frame_time = std::chrono::high_resolution_clock::now();

  uint32_t image_count;
  vkGetSwapchainImagesKHR(m_context.m_device, m_context.m_swapchain, &image_count, nullptr);

  m_image_available_semaphores.resize(m_max_frame_in_flight);
  m_in_flight_fences.resize(m_max_frame_in_flight);
  m_render_finished_semaphores.resize(image_count); 

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < m_max_frame_in_flight; i++) {
    if (vkCreateSemaphore(m_context.m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
        vkCreateFence(m_context.m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create synchronization objects");
    }
  }

  for (size_t i = 0; i < image_count; i++) {
    if (vkCreateSemaphore(m_context.m_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create synchronization objects");
    }
  }
}

IRenderer::IRenderer()
{
  m_context.m_vsync = m_is_vsync;

  n_window::create(m_window);
  n_context::create(m_context, m_window);

  uint32_t imageCount;
  vkGetSwapchainImagesKHR(m_context.m_device, m_context.m_swapchain, &imageCount, nullptr);
  n_descriptor::create(m_descriptor, m_context, m_max_frame_in_flight);
  n_pipeline::create(m_pipeline, m_context, m_descriptor);
  n_commands::create(m_command, m_context, m_max_frame_in_flight, m_thread_count);
  n_buffer::create_vertex_buffer(m_context, m_command, m_buffer);

  create_sync_objects();

  m_last_frame_time = std::chrono::high_resolution_clock::now();
}

IRenderer::~IRenderer()
{
  vkDeviceWaitIdle(m_context.m_device);

  destroy_sync_objects();

  n_buffer::destroy_buffer(m_context, m_buffer);
  n_commands::destroy(m_command);
  n_pipeline::destroy(m_pipeline, m_context);
  n_descriptor::destroy(m_descriptor, m_context);
  n_context::destroy(m_context);
  n_window::destroy(m_window);
}

void IRenderer::recreate_swapchain()
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(m_window.m_handle, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(m_window.m_handle, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(m_context.m_device);

  n_context::destroy_swapchain(m_context);

  destroy_sync_objects();

  n_context::create_swapchain(m_context, m_window);
  n_context::create_image_views(m_context);

  create_sync_objects();

  std::print("resized\n");
}


void IRenderer::draw()
{
    auto frame_start = std::chrono::high_resolution_clock::now();
    delta_time = std::chrono::duration<float>(frame_start - m_last_frame_time).count();
 
  // Wait for the current frame's fence
  vkWaitForFences(m_context.m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

  // Acquire next swapchain image
  uint32_t image_index;
  VkResult result = vkAcquireNextImageKHR(m_context.m_device, 
      m_context.m_swapchain, 
      UINT64_MAX, 
      m_image_available_semaphores[m_current_frame], 
      VK_NULL_HANDLE, 
      &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image");
  }

  // Reset fence for current frame
  vkResetFences(m_context.m_device, 1, &m_in_flight_fences[m_current_frame]);

  //projet an stuff remove later
   ShaderData ubo{};
  ubo.projection = glm::perspective(glm::radians(45.0f),
      (float)m_context.m_swapchain_extent.width / (float)m_context.m_swapchain_extent.height,
      0.1f, 100.0f);
  ubo.projection[1][1] *= -1; // flip Y for Vulkan's clip space
  ubo.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  ubo.model[0] = glm::mat4(1.0f);
  ubo.model[1] = glm::mat4(1.0f);
  ubo.model[2] = glm::mat4(1.0f);
  n_descriptor::update_ubo(m_descriptor, m_current_frame, ubo);

  // Record command buffer
  std::vector<std::thread> workers;
  for (uint32_t t = 0; t < m_thread_count; t++) {
    workers.emplace_back([&, t]() {
        n_commands::record_secondary(m_command, m_context, m_pipeline, m_buffer,
            m_descriptor, m_current_frame, t, 0, 3);
        });
  }
  for (auto &w : workers) w.join();

  n_commands::record_primary(m_command, m_context, m_current_frame, image_index);

  // Submit command buffer
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // waiting 
  VkSemaphore wait_semaphores[] = {m_image_available_semaphores[m_current_frame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = waitStages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &m_command.m_primary_buffers[m_current_frame];

  VkSemaphore signalSemaphores[] = {m_render_finished_semaphores[image_index]};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(m_context.m_graphics_queue, 1, &submit_info, m_in_flight_fences[m_current_frame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer");
  }

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {m_context.m_swapchain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains;
  present_info.pImageIndices = &image_index;

  result = vkQueuePresentKHR(m_context.m_present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.m_is_window_resized) {
    m_window.m_is_window_resized = false;
    recreate_swapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swapchain image");
  }

  m_last_frame_time = frame_start;
  m_current_frame = (m_current_frame + 1) % m_max_frame_in_flight;
}

void IRenderer::destroy_sync_objects()
{
  for (auto s : m_image_available_semaphores)
    if (s != VK_NULL_HANDLE) vkDestroySemaphore(m_context.m_device, s, nullptr);
  for (auto s : m_render_finished_semaphores)
    if (s != VK_NULL_HANDLE) vkDestroySemaphore(m_context.m_device, s, nullptr);
  for (auto f : m_in_flight_fences)
    if (f != VK_NULL_HANDLE) vkDestroyFence(m_context.m_device, f, nullptr);

  m_image_available_semaphores.clear();
  m_render_finished_semaphores.clear();
  m_in_flight_fences.clear();
}
