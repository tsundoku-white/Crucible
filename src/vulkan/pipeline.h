#pragma once

struct Context;

struct Pipeline
{
  VkPipelineLayout m_layout   = VK_NULL_HANDLE;
    VkPipeline       m_pipeline = VK_NULL_HANDLE;
};

namespace n_pipeline
{
  void create_pipeline(Pipeline &pipeline, Context &context);
  void destory_pipeline(Pipeline &pipeline, Context &context);
}
