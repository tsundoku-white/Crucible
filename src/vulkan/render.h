#pragma once
#include "src/pch.h"

#include "src/vulkan/window.h"
#include "src/vulkan/vk_context.h"
#include "src/vulkan/vk_pipeline.h"
#include "src/vulkan/vk_buffer.h"
#include "src/vulkan/vk_command.h"
#include "src/vulkan/vk_descriptor.h"
#include <chrono>
#include <cstdint>

// this is the main acess point of the rendering,
// any and all rendering code that has vulkan or drawing will be here.

// rendering master class
class IRenderer 
{
  public:
    IRenderer();
    ~IRenderer();

    float delta_time = 0.f;

    bool should_close();
    void draw();
  private:
    // memeber objects
    Vk_Window     m_window;
    Vk_Context    m_context;
    Vk_Pipeline   m_pipeline;
    Vk_Buffer     m_buffer;
    Vk_Command    m_command;
    Vk_Descriptor m_descriptor;

    // vulkan frames in flight
    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence>     m_in_flight_fences;
    uint32_t m_current_frame = 0;

    // time
    std::chrono::time_point<std::chrono::high_resolution_clock> m_last_frame_time;

    //helper functions
    void recreate_swapchain();
    void create_sync_objects();
    void destroy_sync_objects();

    // settings
    uint32_t m_thread_count = 2;
    uint32_t m_max_frame_in_flight = 3;

    bool m_is_vsync = true;
};
