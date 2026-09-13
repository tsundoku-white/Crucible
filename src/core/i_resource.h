#pragma once

#include "src/core/pch.h"
#include "src/ecs/registery.h"
#include "src/vulkan/buffer.h"
#include "src/vulkan/command.h"
#include <src/core/i_render.h>
#include <src/vulkan/context.h>
#include <src/vulkan/descriptor.h>
#include <sys/types.h>
#include "src/vulkan/image.h"

struct Transform;
struct Camera;
struct Model;

// camera projection and view matrix
struct alignas(16) UniformBufferObject
{
  glm::mat4 m_projectionMatrix;
  glm::mat4 m_viewMatrix;
};

// model matrix and instance count
struct alignas(16) ShaderStorageBufferObject
{
  glm::mat4 m_modelsMatrix    = glm::mat4(1.f);
  uint32_t  m_texture_index   = 0;
  uint32_t  _pad[3];
};
static_assert(sizeof(ShaderStorageBufferObject) == 80);

struct IResource
{
  // copyed variable
  Context    *m_context;
  Registery  *m_registery;

  // owned variable
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

  // buffers
  Buffer vertexBuffer;
  Buffer indexBuffer;
  Buffer uboBuffer;
  Buffer ssboBuffer;
  Texture m_texture;

  // ubos & ssbo arrays
  std::vector<UniformBufferObject       > ubos;
  std::vector<ShaderStorageBufferObject > ssbos;

  // arrays for the ssbo "models" and all textures
  std::vector<ShaderStorageBufferObject> m_instances;
  std::vector<Texture  > m_textures;
  std::vector<uint32_t > m_materialTextureIndex;
};

namespace n_resource
{
  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender);
  void renderResourceUpdate(IResource &iResource, IRender &iRender);
  void destroyResource(IResource &iResource);
}
