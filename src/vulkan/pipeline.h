#pragma once

#include <src/vulkan/descriptor.h>
struct Context;

struct Pipeline
{
  VkPipelineLayout      m_layout           = VK_NULL_HANDLE;
  VkPipeline            m_pipeline         = VK_NULL_HANDLE;
};

namespace n_pipeline
{
  void createPipeline(Pipeline &pipeline, Descriptor &descriptor, Context &context);
  void destoryPipeline(Pipeline &pipeline, Context &context);
}
