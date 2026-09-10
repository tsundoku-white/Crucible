#include "command.h"
#include <cstdint>
#include <cstdio>
#include <src/core/i_render.h>
#include <src/core/pch.h>
#include <src/vulkan/context.h>
#include <src/vulkan/pipeline.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/buffer.h>
#include <array>
#include <stdexcept>
#include <glm/vec3.hpp>

namespace n_command
{
  void createCommand(Command &command, Context &context, IRender &render)
  {
    VkCommandPoolCreateInfo commandPoolCreateInfo {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = context.m_queueFamily;

    vkCheck(vkCreateCommandPool(context.m_device, &commandPoolCreateInfo, nullptr, &command.m_pool), 
        "failed to create pool\n");

    command.m_buffers.resize(render.m_maxFramesInFlight);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = command.m_pool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = render.m_maxFramesInFlight;

    vkCheck(vkAllocateCommandBuffers(context.m_device, &commandBufferAllocateInfo, command.m_buffers.data()), 
        "failed to create pool buffer\n");
  }

  void destroyCommand(Command &command, Context &context)
  {
    if (command.m_pool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(context.m_device, command.m_pool, nullptr);
    }
  }

  void recordPrimary(Command &command, Context &context, Pipeline &pipeline, Descriptor &descriptor,
      Buffer &vertexBuffer, Buffer &indexBuffer, std::vector<Buffer> &shaderDataBuffers,
      uint32_t frameIndex, uint32_t imageIndex, std::vector<DrawInfo> &drawInfos)
  {
    VkCommandBuffer commandBuffer = command.m_buffers[frameIndex];
    g_frameStats.m_drawCalls = 0;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    vkCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), 
        "failed to begin recording command buffer");

    VkImageMemoryBarrier2 colorOutputBarrier{};
    colorOutputBarrier.sType          = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    colorOutputBarrier.srcStageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorOutputBarrier.srcAccessMask  = 0;
    colorOutputBarrier.dstStageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorOutputBarrier.dstAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorOutputBarrier.oldLayout                    = VK_IMAGE_LAYOUT_UNDEFINED;
    colorOutputBarrier.newLayout                    = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorOutputBarrier.image                        = context.m_swapchainImages[imageIndex];
    colorOutputBarrier.subresourceRange.aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
    colorOutputBarrier.subresourceRange.levelCount  = 1;
    colorOutputBarrier.subresourceRange.layerCount  = 1;

    VkImageMemoryBarrier2 depthOutputBarrier{};
    depthOutputBarrier.sType          = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    depthOutputBarrier.srcStageMask   = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    depthOutputBarrier.srcAccessMask  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthOutputBarrier.dstStageMask   = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    depthOutputBarrier.dstAccessMask  = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthOutputBarrier.oldLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
    depthOutputBarrier.newLayout      = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    depthOutputBarrier.image          = context.m_depthImage;
    depthOutputBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depthOutputBarrier.subresourceRange.levelCount = 1;
    depthOutputBarrier.subresourceRange.layerCount = 1;

    std::array<VkImageMemoryBarrier2, 2> outputBarriers{ colorOutputBarrier, depthOutputBarrier };

    VkDependencyInfo barrierDependencyInfo{};
    barrierDependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrierDependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(outputBarriers.size());
    barrierDependencyInfo.pImageMemoryBarriers    = outputBarriers.data();
    vkCmdPipelineBarrier2(commandBuffer, &barrierDependencyInfo);

    VkClearValue clearColor;
    clearColor.color = {{0.07, 0.07, 0.07, 1.0}};

    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView   = context.m_swapchainImageViews[imageIndex];
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue  = clearColor;

    VkRenderingAttachmentInfo depthAttachmentInfo{};
    depthAttachmentInfo.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachmentInfo.imageView   = context.m_depthImageView;
    depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    depthAttachmentInfo.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentInfo.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType                     = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent.width   = context.m_swapchain_extent.width;
    renderingInfo.renderArea.extent.height  = context.m_swapchain_extent.height;
    renderingInfo.layerCount                = 1;
    renderingInfo.colorAttachmentCount      = 1;
    renderingInfo.pColorAttachments         = &colorAttachmentInfo;
    renderingInfo.pDepthAttachment          = &depthAttachmentInfo;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport vp{};
    vp.width    = static_cast<float>(context.m_swapchain_extent.width);
    vp.height   = static_cast<float>(context.m_swapchain_extent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &vp);

    VkRect2D scissor{};
    scissor.extent.width  = context.m_swapchain_extent.width;
    scissor.extent.height = context.m_swapchain_extent.height; 

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_pipeline);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_layout,
        0, 1, &descriptor.m_sets[frameIndex], 0, nullptr);

    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.m_buffer, &vOffset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.m_buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdPushConstants(commandBuffer, pipeline.m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
        sizeof(VkDeviceAddress), &shaderDataBuffers[0].m_address);

    for (const auto &draw : drawInfos)
    {
      vkCmdDrawIndexed(commandBuffer, draw.index_count, draw.instance_count,
          draw.first_index, draw.vertex_offset, draw.first_instance);
      g_frameStats.m_drawCalls++;
    }
    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier2 barrierPresent{};
    barrierPresent.sType          = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrierPresent.srcStageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrierPresent.srcAccessMask  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrierPresent.dstStageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrierPresent.dstAccessMask  = 0;
    barrierPresent.oldLayout      = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    barrierPresent.newLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrierPresent.image          = context.m_swapchainImages[imageIndex];
    barrierPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrierPresent.subresourceRange.levelCount = 1;
    barrierPresent.subresourceRange.layerCount = 1;

    VkDependencyInfo barrierPresentDependencyInfo{};
    barrierPresentDependencyInfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    barrierPresentDependencyInfo.imageMemoryBarrierCount  = 1;
    barrierPresentDependencyInfo.pImageMemoryBarriers     = &barrierPresent;
    vkCmdPipelineBarrier2(commandBuffer, &barrierPresentDependencyInfo);

    vkCheck(vkEndCommandBuffer(commandBuffer),"failed to record command buffer");
  }

  VkCommandBuffer beginSingleTime(Command &command, Context &context)
  {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command.m_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(context.m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
  }

  void endSingleTime(Command &command, Context &context, VkCommandBuffer commandBuffer)
  {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context.m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.m_queue);

    vkFreeCommandBuffers(context.m_device, command.m_pool, 1, &commandBuffer);
  }
}
