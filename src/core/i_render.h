#pragma once

#include "src/core/pch.h"
#include <src/vulkan/pipeline.h>
#include <chrono>

// forward decloration
struct Context;
struct Window;
struct IResource;

struct IRender
{
  // copyed varible
  Context *m_context = nullptr;
  Window  *m_window  = nullptr;

  static constexpr uint32_t m_maxFramesInFlight = 2;

  double   m_deltaTime  = 0.0f;
  uint32_t m_frameIndex = 0;
  bool     m_vsync      = false;

  std::vector<VkFence    > m_fences{};
  std::vector<VkSemaphore> m_imageAcquiredSemaphores{};
  std::vector<VkSemaphore> m_renderCompleteSemaphores{};
  std::chrono::time_point<std::chrono::high_resolution_clock> m_last_frame_time;
};

namespace n_render
{
  void createSyncObjects(IRender &iRender);
  void createIRender(IRender &iRender, Context &context, Window &window);
  void drawIRender(IRender &iRender, IResource &iResource);
  void recreateSwapchain(IRender &iRender);
  void destoryIRender(IRender &iRender);
}
