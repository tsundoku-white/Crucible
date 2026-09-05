#pragma once

#include "src/core/pch.h"
#include "src/ecs/registery.h"
#include "src/vulkan/buffer.h"
#include "src/vulkan/command.h"
#include <memory>
#include <src/core/i_render.h>
#include <src/vulkan/context.h>
#include <src/vulkan/descriptor.h>

struct Transform;
struct Camera;
struct Model;

struct IResource
{
  Context    *m_context;
  Registery  *m_registery;
  Command     m_command;
  Descriptor  m_descriptor;

  std::unordered_map<EntityID, bool> m_has_transform;
  std::unordered_map<EntityID, bool> m_has_camera;
  std::unordered_map<EntityID, bool> m_has_model;

  std::unordered_map<EntityID, Transform*>  m_transform_cache;
  std::unordered_map<EntityID, Camera*>     m_camera_cache;
  std::unordered_map<EntityID, Model*>      m_model_cache;

  bool m_dirty_transform  = false;
  bool m_dirty_camera     = false;
  bool m_dirty_model      = false;

  Buffer vertexBuffer;
  Buffer indexBuffer;

  std::vector<Buffer> uboBuffer;
  std::vector<Buffer> ssboBuffer;
  std::vector<UniformBufferObject> ubos;
  std::vector<ShaderStorageBufferObject> ssbos;

};

namespace n_resource
{
  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender);
  void renderResourceUpdate(IResource &iResource, IRender &iRender);
  void destroyResource(IResource &iResource);
}
