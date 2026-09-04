#pragma once

#include "src/core/pch.h"

struct Context;
struct Pipeline;
struct IRender;
struct Descriptor;
struct Buffer;

struct DrawInfo 
{
  uint32_t index_count    = 0;
  uint32_t instance_count = 1;
  uint32_t first_index    = 0;
  int32_t  vertex_offset  = 0;
  uint32_t first_instance = 0;
  uint32_t instance_index = 0;
};

struct Command
{
  VkCommandPool                 m_pool    = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer>  m_buffers = {};
};

namespace n_command
{
  void createCommand(Command &command, Context &context, IRender &render);
  void destroyCommand(Command &command, Context &context);

  // vertexBuffer / indexBuffer: the actual mesh data to draw.
  // shaderDataBuffers: per-frame buffer holding a VkDeviceAddress pushed to the shader
  //   (must have at least frameIndex+1 entries; index it the same way command.m_buffers is indexed).
  void recordPrimary(Command &command, Context &context, Pipeline &pipeline, Descriptor &descriptor,
      Buffer &vertexBuffer, Buffer &indexBuffer, std::vector<Buffer> &shaderDataBuffers,
      uint32_t frameIndex, uint32_t imageIndex, std::vector<DrawInfo> &drawInfos);
}
