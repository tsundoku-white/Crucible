#include "i_render.h"

#include "src/vulkan/context.h"
#include "src/vulkan/window.h"
#include <src/core/i_resource.h>
#include <src/vulkan/pipeline.h>

namespace n_render
{
  void createSyncObjects(IRender &iRender)
  {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(iRender.m_context->m_device,
        iRender.m_context->m_swapchain,
        &imageCount, nullptr);

    iRender.m_imageAcquiredSemaphores.resize(IRender::m_maxFramesInFlight);
    iRender.m_fences.resize(IRender::m_maxFramesInFlight);
    iRender.m_renderCompleteSemaphores.resize(imageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < iRender.m_maxFramesInFlight; i++) {
    if (vkCreateSemaphore(iRender.m_context->m_device, &semaphoreInfo, nullptr, 
          &iRender.m_imageAcquiredSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(iRender.m_context->m_device, &fenceInfo, nullptr, 
          &iRender.m_fences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create synchronization objects");
    }
  }

  for (size_t i = 0; i < imageCount; i++) {
    if (vkCreateSemaphore(iRender.m_context->m_device, &semaphoreInfo, nullptr, 
          &iRender.m_renderCompleteSemaphores[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create synchronization objects");
    }
  }
  }

  void recreateSwapchain(IRender &iRender)
  {
    int width = 0, height = 0;
    glfwGetFramebufferSize(iRender.m_window->m_handle, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(iRender.m_window->m_handle, &width, &height);
      glfwWaitEvents();
    }

    vkDeviceWaitIdle(iRender.m_context->m_device);

    n_context::destroySwapchain(*iRender.m_context);
    n_context::createSwapchain(*iRender.m_context, *iRender.m_window);
  }

  void createIRender(IRender &iRender, Context &context, Window &window)
  {
    iRender.m_context = &context;
    iRender.m_window  = &window;

    n_pipeline::createPipeline(iRender.m_pipeline, *iRender.m_context);
    createSyncObjects(iRender);
  }

  void drawIRender(IRender &iRender, IResource &iResource)
  {
    // Wait for previous frame
    vkWaitForFences(iRender.m_context->m_device, 1,
        &iRender.m_fences[iRender.m_frameIndex], true, UINT64_MAX);

    // Acquire next image from swapchain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        iRender.m_context->m_device,
        iRender.m_context->m_swapchain,
        UINT64_MAX,
        iRender.m_imageAcquiredSemaphores[iRender.m_frameIndex],
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapchain(iRender);
      return;
    }
    vkResetFences(iRender.m_context->m_device, 1, 
        &iRender.m_fences[iRender.m_frameIndex]);

    // Update resource data (UBO, SSBO)
    n_resource::renderResourceUpdate(iResource, iRender);

    // Record commands
    std::vector<DrawInfo> drawInfos;
    DrawInfo drawInfo{};
    drawInfo.index_count = iResource.indexBuffer.m_size / sizeof(uint32_t);
    drawInfo.instance_count = 1;
    drawInfo.first_index = 0;
    drawInfo.vertex_offset = 0;
    drawInfo.first_instance = 0;
    drawInfos.push_back(drawInfo);

    n_command::recordPrimary(
        iResource.m_command,
        *iRender.m_context,
        iRender.m_pipeline,
        iResource.m_descriptor,
        iResource.vertexBuffer,
        iResource.indexBuffer,
        iResource.uboBuffer,
        iRender.m_frameIndex,
        imageIndex,
        drawInfos
        );

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {
      iRender.m_imageAcquiredSemaphores[iRender.m_frameIndex]
    };
    VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &iResource.m_command.m_buffers[iRender.m_frameIndex];

    VkSemaphore signalSemaphores[] = {
      iRender.m_renderCompleteSemaphores[iRender.m_frameIndex]
    };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(iRender.m_context->m_queue, 1, &submitInfo, 
          iRender.m_fences[iRender.m_frameIndex]) != VK_SUCCESS) {
      throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = {iRender.m_context->m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(iRender.m_context->m_queue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      recreateSwapchain(iRender);
    }  else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swapchain image");
    }

    // Advance frame
    iRender.m_frameIndex = (iRender.m_frameIndex + 1) % iRender.m_maxFramesInFlight;
  }

  void destoryIRender(IRender &iRender)
  {
    n_pipeline::destoryPipeline(iRender.m_pipeline, *iRender.m_context);

    for (auto s : iRender.m_imageAcquiredSemaphores)
      if (s != VK_NULL_HANDLE) vkDestroySemaphore(iRender.m_context->m_device, s, nullptr);
    for (auto s : iRender.m_renderCompleteSemaphores)
      if (s != VK_NULL_HANDLE) vkDestroySemaphore(iRender.m_context->m_device, s, nullptr);
    for (auto f : iRender.m_fences)
      if (f != VK_NULL_HANDLE) vkDestroyFence(iRender.m_context->m_device, f, nullptr);

    iRender.m_imageAcquiredSemaphores.clear();
    iRender.m_renderCompleteSemaphores.clear();
    iRender.m_fences.clear();
  }
}
