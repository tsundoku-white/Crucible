#include "vk_command.h"
#include "src/vulkan/vk_context.h"
#include "src/vulkan/vk_descriptor.h"
#include "src/vulkan/vk_pipeline.h"
#include "src/vulkan/vk_buffer.h"

namespace n_commands
{
  void create(Vk_Command &cmd, Vk_Context &context, uint32_t frame_count, uint32_t thread_count)
  {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context.m_physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(context.m_physical_device, &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_family_count; i++) {
      if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        cmd.m_queue_family_index = i;
        break;
      }
    }

    cmd.m_device       = context.m_device;
    cmd.m_thread_count = thread_count;

    cmd.m_primary_pools.resize(frame_count);
    cmd.m_primary_buffers.resize(frame_count);
    cmd.m_thread_pools.resize(frame_count);
    cmd.m_secondary_buffers.resize(frame_count);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = cmd.m_queue_family_index;

    for (uint32_t f = 0; f < frame_count; f++) {
      // primary pool + buffer for this frame
      if (vkCreateCommandPool(context.m_device, &pool_info, nullptr, &cmd.m_primary_pools[f]) != VK_SUCCESS)
        throw std::runtime_error("failed to create primary command pool!");

      VkCommandBufferAllocateInfo primary_alloc{};
      primary_alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      primary_alloc.commandPool        = cmd.m_primary_pools[f];
      primary_alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      primary_alloc.commandBufferCount = 1;

      if (vkAllocateCommandBuffers(context.m_device, &primary_alloc, &cmd.m_primary_buffers[f]) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate primary command buffer!");

      // one pool + one secondary buffer per thread, for this frame
      cmd.m_thread_pools[f].resize(thread_count);
      cmd.m_secondary_buffers[f].resize(thread_count);

      for (uint32_t t = 0; t < thread_count; t++) {
        if (vkCreateCommandPool(context.m_device, &pool_info, nullptr, &cmd.m_thread_pools[f][t]) != VK_SUCCESS)
          throw std::runtime_error("failed to create thread command pool!");

        VkCommandBufferAllocateInfo sec_alloc{};
        sec_alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        sec_alloc.commandPool        = cmd.m_thread_pools[f][t];
        sec_alloc.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        sec_alloc.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(context.m_device, &sec_alloc, &cmd.m_secondary_buffers[f][t]) != VK_SUCCESS)
          throw std::runtime_error("failed to allocate secondary command buffer!");
      }
    }

    std::print("\e[0;32m" "pass: " "\e[0m" "vk command (threaded)\n");
  }

  void destroy(Vk_Command &cmd)
  {
    for (auto pool : cmd.m_primary_pools)
      if (pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(cmd.m_device, pool, nullptr);

    for (auto &frame_pools : cmd.m_thread_pools)
      for (auto pool : frame_pools)
        if (pool != VK_NULL_HANDLE)
          vkDestroyCommandPool(cmd.m_device, pool, nullptr);
  }

  static void transition_image(VkCommandBuffer cmd_buf, VkImage image,
      VkImageLayout old_layout, VkImageLayout new_layout,
      VkAccessFlags2 src_access, VkAccessFlags2 dst_access,
      VkPipelineStageFlags2 src_stage, VkPipelineStageFlags2 dst_stage)
  {
    VkImageMemoryBarrier2 barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask        = src_stage;
    barrier.srcAccessMask       = src_access;
    barrier.dstStageMask        = dst_stage;
    barrier.dstAccessMask       = dst_access;
    barrier.oldLayout           = old_layout;
    barrier.newLayout           = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkDependencyInfo dep_info{};
    dep_info.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers    = &barrier;

    vkCmdPipelineBarrier2(cmd_buf, &dep_info);
  }

  // called on a worker thread — records this thread's slice of the draw calls
  void record_secondary(Vk_Command &cmd, Vk_Context &context, Vk_Pipeline &pip,
      Vk_Buffer &vertex_buffer, Vk_Descriptor &descriptor, uint32_t frame_index, uint32_t thread_index,
      uint32_t first_vertex, uint32_t vertex_count)
  {
    VkCommandBuffer sec_buf = cmd.m_secondary_buffers[frame_index][thread_index];

    // must reset this thread's pool before re-recording (we set RESET_COMMAND_BUFFER_BIT,
    // so resetting the individual buffer is fine too — either works)
    vkResetCommandBuffer(sec_buf, 0);

    VkCommandBufferInheritanceRenderingInfo inheritance_rendering{};
    inheritance_rendering.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
    inheritance_rendering.colorAttachmentCount    = 1;
    inheritance_rendering.pColorAttachmentFormats = &context.m_swapchain_image_format;
    inheritance_rendering.rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT;

    VkCommandBufferInheritanceInfo inheritance{};
    inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance.pNext = &inheritance_rendering;

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = &inheritance;

    if (vkBeginCommandBuffer(sec_buf, &begin_info) != VK_SUCCESS)
      throw std::runtime_error("failed to begin secondary command buffer!");

    vkCmdBindPipeline(sec_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pip.m_graphics);

    int instance_index = 0; 
    vkCmdPushConstants(sec_buf, pip.m_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(int), &instance_index);

    vkCmdBindDescriptorSets(sec_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pip.m_layout,
    0, 1, &descriptor.m_sets[frame_index], 0, nullptr);

    // viewport/scissor aren't inherited from the primary — each secondary needs them if dynamic
    VkViewport viewport{0.0f, 0.0f,
      static_cast<float>(context.m_swapchain_extent.width),
      static_cast<float>(context.m_swapchain_extent.height), 0.0f, 1.0f};
    vkCmdSetViewport(sec_buf, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, context.m_swapchain_extent};
    vkCmdSetScissor(sec_buf, 0, 1, &scissor);

    VkBuffer     vertex_buffers[] = {vertex_buffer.m_buffer};
    VkDeviceSize offsets[]        = {0};
    vkCmdBindVertexBuffers(sec_buf, 0, 1, vertex_buffers, offsets);

    vkCmdDraw(sec_buf, vertex_count, 1, first_vertex, 0);

    if (vkEndCommandBuffer(sec_buf) != VK_SUCCESS)
      throw std::runtime_error("failed to end secondary command buffer!");
  }

  // called on the main thread AFTER all worker threads have finished record_secondary for this frame
  void record_primary(Vk_Command &cmd, Vk_Context &context, uint32_t frame_index, uint32_t image_index)
  {
    VkCommandBuffer primary = cmd.m_primary_buffers[frame_index];

    vkResetCommandBuffer(primary, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(primary, &begin_info) != VK_SUCCESS)
      throw std::runtime_error("failed to begin primary command buffer!");

    VkImage image = context.m_swapchain_images[image_index];

    transition_image(primary, image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        0, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    VkRenderingAttachmentInfo color_attachment{};
    color_attachment.sType             = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.imageView         = context.m_swapchain_image_views[image_index];
    color_attachment.imageLayout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp            = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp           = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color  = {{0.01f, 0.01f, 0.02f, 1.0f}};

    VkRenderingInfo rendering_info{};
    rendering_info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea           = {{0, 0}, context.m_swapchain_extent};
    rendering_info.layerCount           = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments    = &color_attachment;
    // KEY: tells Vulkan this render pass will contain secondary buffers only, not inline draws
    rendering_info.flags                = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;

    vkCmdBeginRendering(primary, &rendering_info);

    vkCmdExecuteCommands(primary,
        static_cast<uint32_t>(cmd.m_secondary_buffers[frame_index].size()),
        cmd.m_secondary_buffers[frame_index].data());

    vkCmdEndRendering(primary);

    transition_image(primary, image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    if (vkEndCommandBuffer(primary) != VK_SUCCESS)
      throw std::runtime_error("failed to record primary command buffer!");
  }
}
