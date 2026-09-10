#pragma once

#include "src/core/pch.h"
#include "src/ecs/registery.h"
#include "src/vulkan/buffer.h"
#include "src/vulkan/command.h"
#include <src/core/i_render.h>
#include <src/vulkan/context.h>
#include <src/vulkan/descriptor.h>
#include "src/vulkan/image.h"

struct Transform;
struct Camera;
struct Model;

struct alignas(16) UniformBufferObject
{
  // camera projection
  glm::mat4 m_projectionMatrix;
  glm::mat4 m_viewMatrix;
};

struct alignas(16) ShaderStorageBufferObject
{
  // models
  glm::mat4 m_modelsMatrix = glm::mat4(1.f);
  size_t    m_modelCount   = 0;
};

struct IResource
{
  Context    *m_context;
  Registery  *m_registery;

  Pipeline    m_pipeline; 
  Command     m_command;
  Descriptor  m_descriptor;

  bool m_dirty_transform  = false;
  bool m_dirty_camera     = false;
  bool m_dirty_projection = false;
  bool m_dirty_model      = false;
  bool m_dirty_cache      = false;
  bool m_isBufferCreated  = false;

  uint32_t m_total_shared_index_count = 0;
  uint32_t m_modelInstanceCount = 0;

  Buffer vertexBuffer;
  Buffer indexBuffer;
  Buffer uboBuffer;
  Buffer ssboBuffer;
  Texture m_texture;

  std::vector<UniformBufferObject       > ubos;
  std::vector<ShaderStorageBufferObject > ssbos;

  std::vector<glm::mat4> m_modelMatrices;
  std::vector<int32_t  > m_textures;
};

namespace n_resource
{
  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender);
  void renderResourceUpdate(IResource &iResource, IRender &iRender);
  void destroyResource(IResource &iResource);
}
