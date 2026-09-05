#pragma once

struct Context;

struct Pipeline
{
  VkPipelineLayout      m_layout           = VK_NULL_HANDLE;
  VkPipeline            m_pipeline         = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_descriptorLayout = VK_NULL_HANDLE;
};

namespace n_pipeline
{
  void createPipeline(Pipeline &pipeline, Context &context);
  void destoryPipeline(Pipeline &pipeline, Context &context);
}
