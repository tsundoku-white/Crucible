#pragma once
#include "src/pch.h"
#include "src/vulkan/vk_buffer.h"
#include "src/vulkan/vk_descriptor.h"

struct Vk_Context;

struct Vk_Pipeline 
{
    VkPipelineLayout           m_layout;
    VkPipeline                 m_graphics;
};

namespace n_pipeline
{
  void create(Vk_Pipeline &pipeline, Vk_Context &context, Vk_Descriptor &decscriptor);
  void destroy(Vk_Pipeline &pipeline, Vk_Context &vk_context);
}
