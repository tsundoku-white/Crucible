#pragma once

#include "src/core/pch.h"
#include <src/vulkan/pipeline.h>

struct Context;
struct Window;
struct IResource;

struct IRender
{
  Context *m_context = nullptr;
  Window  *m_window  = nullptr;

  Pipeline m_pipeline;
  static constexpr uint32_t m_maxFramesInFlight = 2;

  uint32_t m_frameIndex = 0;
  bool     m_vsync      = false;

  std::vector<VkFence    > m_fences{};
  std::vector<VkSemaphore> m_imageAcquiredSemaphores{};
  std::vector<VkSemaphore> m_renderCompleteSemaphores{};
};

namespace n_render
{
  void createSyncObjects(IRender &iRender);
  void createIRender(IRender &iRender, Context &context, Window &window);
  void drawIRender(IRender &iRender, IResource &iResource);
  void recreateSwapchain(IRender &iRender);
  void destoryIRender(IRender &iRender);
}
