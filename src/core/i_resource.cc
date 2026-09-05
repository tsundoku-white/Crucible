#include "src/core/i_resource.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <src/core/i_render.h>
#include <src/core/pch.h>
#include <src/ecs/entity.h>
#include <src/vulkan/descriptor.h>
#include "src/ecs/components/transform.h"
#include "src/ecs/components/camera.h"
#include "src/ecs/components/model.h"
#include "src/vulkan/buffer.h"
#include "src/vulkan/context.h"

namespace n_resource
{
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

    // textures
    int32_t   m_indices       = 0;
    size_t    m_indicesCount  = 0;
  };

  void updateCache(IResource &iResource, Registery &registery, Context &context, IRender &iRender)
  {
    auto &all_entities = registery.getEntityView();

    // check change
    if (iResource.m_has_transform.size() != all_entities.size())
    {
      createResource(iResource, registery, context, iRender);
      return;
    }

    for (size_t e = 0; e < all_entities.size(); e++)
    {
      EntityID id = all_entities[e];

      bool current_has_transform = registery.has<Transform>(id);
      bool current_has_camera = registery.has<Camera>(id);
      bool current_has_model = registery.has<Model>(id);

      if (iResource.m_has_transform[id] != current_has_transform)
      {
        iResource.m_has_transform[id] = current_has_transform;
        iResource.m_dirty_transform = true;

        if (current_has_transform)
          iResource.m_transform_cache[id] = &registery.get<Transform>(id);
        else
          iResource.m_transform_cache.erase(id);
      }

      if (iResource.m_has_camera[id] != current_has_camera)
      {
        iResource.m_has_camera[id] = current_has_camera;
        iResource.m_dirty_camera = true;

        if (current_has_camera)
          iResource.m_camera_cache[id] = &registery.get<Camera>(id);
        else
          iResource.m_camera_cache.erase(id);
      }

      if (iResource.m_has_model[id] != current_has_model)
      {
        iResource.m_has_model[id] = current_has_model;
        iResource.m_dirty_model = true;

        if (current_has_model)
          iResource.m_model_cache[id] = &registery.get<Model>(id);
        else
          iResource.m_model_cache.erase(id);
      }
    }
  }

  void transformUpdate(IResource &iResource, EntityID id)
  {
    auto it = iResource.m_transform_cache.find(id);
    if (it != iResource.m_transform_cache.end() && iResource.m_dirty_transform)
    {
      auto *transform = it->second;

      if (transform)
      {
        transform->m_forward  = transform->m_rotation * glm::vec3(0, 0, -1);
        transform->m_up       = transform->m_rotation * glm::vec3(0, 1, 0);
        transform->m_right    = transform->m_rotation * glm::vec3(1, 0, 0);
      }
      iResource.m_dirty_transform = false;
    }
  }

  void cameraUpdate(IResource &iResource, IRender &iRender, EntityID id)
  {
    auto transform_it = iResource.m_transform_cache.find(id);
    auto camera_it = iResource.m_camera_cache.find(id);

    if (camera_it != iResource.m_camera_cache.end() && 
        transform_it != iResource.m_transform_cache.end() && 
        iResource.m_dirty_camera)
    {
      auto *camera = camera_it->second;
      auto *transform = transform_it->second;

      if (camera && transform)
      {
        // Make sure ubos vector has enough elements
        if (iResource.ubos.empty())
          iResource.ubos.resize(1);

        float aspect = static_cast<float>(iRender.m_context->m_swapchain_extent.width) / 
          static_cast<float>(iRender.m_context->m_swapchain_extent.height);

        iResource.ubos[0].m_projectionMatrix = glm::perspective(
            camera->m_fov,
            aspect,
            camera->m_minViewDistance,
            camera->m_maxViewDistance
            );

        iResource.ubos[0].m_projectionMatrix[1][1] *= -1;

        iResource.ubos[0].m_viewMatrix = glm::lookAt(
            transform->m_location,
            transform->m_forward + transform->m_location,
            transform->m_up
            );
      }
    }
    iResource.m_dirty_camera = false;
  }

  void modelUpdate(IResource &iResource, EntityID id)
  {
    auto model_it = iResource.m_model_cache.find(id);
    auto transform_it = iResource.m_transform_cache.find(id);

    if (model_it != iResource.m_model_cache.end() && 
        transform_it != iResource.m_transform_cache.end() && 
        iResource.m_dirty_model)
    {
      auto *model = model_it->second;
      auto *transform = transform_it->second;

      if (model && transform)
      {
        // Make sure ssbos vector has enough elements
        if (iResource.ssbos.empty())
          iResource.ssbos.resize(1);

        glm::mat4 matrix(1.f);

        matrix = glm::translate(matrix, transform->m_location);
        matrix = matrix * glm::toMat4(transform->m_rotation);
        matrix = glm::scale(matrix, transform->m_scale);

        iResource.ssbos[0].m_modelsMatrix = matrix;
      }
    }
    iResource.m_dirty_model = false;
  }

  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender)
  {
    auto &all_entities = registery.getEntityView();

    iResource.m_context   = &context;
    iResource.m_registery = &registery;

    // size the cache for less runtime calc.
    iResource.m_has_transform.reserve(all_entities.size());
    iResource.m_has_camera.reserve(all_entities.size());
    iResource.m_has_model.reserve(all_entities.size());

    iResource.m_transform_cache.reserve(all_entities.size());
    iResource.m_camera_cache.reserve(all_entities.size());
    iResource.m_model_cache.reserve(all_entities.size());

    // Clear existing data
    iResource.m_has_transform.clear();
    iResource.m_has_camera.clear();
    iResource.m_has_model.clear();
    iResource.m_transform_cache.clear();
    iResource.m_camera_cache.clear();
    iResource.m_model_cache.clear();

    n_command::createCommand(iResource.m_command, context, iRender);
    
    for (size_t e = 0; e < all_entities.size(); e++)
    {
      EntityID id = all_entities[e];

      bool has_transform  = registery.has<Transform>(id);
      bool has_camera     = registery.has<Camera>(id);
      bool has_model      = registery.has<Model>(id);

      iResource.m_has_transform[id] = has_transform;
      iResource.m_has_camera[id]    = has_camera;
      iResource.m_has_model[id]     = has_model;

      if (has_model)
      {
        auto *model = &registery.get<Model>(id);
        iResource.m_model_cache[id] = model;

        // Create vertex and index buffers for this model
        n_buffer::createVertexBuffer(iResource.vertexBuffer, iResource.m_command, context,
            model->m_mesh_meta_data.m_vertex_data);
        n_buffer::createIndexBuffer(iResource.indexBuffer, iResource.m_command, context,
            model->m_mesh_meta_data.m_index_data);
      }

      if (has_transform)
        iResource.m_transform_cache[id] = &registery.get<Transform>(id);

      if (has_camera)
        iResource.m_camera_cache[id] = &registery.get<Camera>(id);
    }

    // Initialize UBO and SSBO vectors
    iResource.ubos.resize(1);
    iResource.ssbos.resize(1);

    // Create buffers 
    iResource.uboBuffer.resize(1);
    iResource.ssboBuffer.resize(1);

    n_buffer::createUniformBuffer(iResource.uboBuffer[0], context, sizeof(UniformBufferObject));
    n_buffer::createStorageBuffer(iResource.ssboBuffer[0], context, sizeof(ShaderStorageBufferObject));

    n_descriptor::createDescriptorSets(iResource.m_descriptor, context, iRender.m_pipeline.m_descriptorLayout,
        iResource.uboBuffer[0], iResource.ssboBuffer[0], iRender.m_maxFramesInFlight);
  }

  void renderResourceUpdate(IResource &iResource, IRender &iRender)
  {
    auto &all_entities = iResource.m_registery->getEntityView();

    for (size_t e = 0; e < all_entities.size(); e++)
    {
      EntityID id = all_entities[e]; 

      updateCache(iResource, *iResource.m_registery, *iResource.m_context, iRender);
      transformUpdate(iResource, id);
      cameraUpdate(iResource, iRender, id);
      modelUpdate(iResource, id);

      if (!iResource.ubos.empty() && !iResource.uboBuffer.empty())
      {
        void* data;
        vmaMapMemory(iRender.m_context->m_allocator, 
            iResource.uboBuffer[0].m_allocation, &data);
        memcpy(data, &iResource.ubos[0], sizeof(UniformBufferObject));
        vmaUnmapMemory(iRender.m_context->m_allocator, 
            iResource.uboBuffer[0].m_allocation);
      }

      if (!iResource.ssbos.empty() && !iResource.ssboBuffer.empty())
      {
        void* data;
        vmaMapMemory(iRender.m_context->m_allocator, 
            iResource.ssboBuffer[0].m_allocation, &data);
        memcpy(data, &iResource.ssbos[0], sizeof(ShaderStorageBufferObject));
        vmaUnmapMemory(iRender.m_context->m_allocator, 
            iResource.ssboBuffer[0].m_allocation);
      }
    }
  }

  void destroyResource(IResource &iResource)
  {
    // Clear caches
    iResource.m_has_transform.clear();
    iResource.m_has_camera.clear();
    iResource.m_has_model.clear();
    iResource.m_transform_cache.clear();
    iResource.m_camera_cache.clear();
    iResource.m_model_cache.clear();

    n_command::destroyCommand(iResource.m_command , *iResource.m_context);
    n_descriptor::destoryDescriptor(iResource.m_descriptor, *iResource.m_context);
    n_buffer::destroyBuffer(iResource.vertexBuffer, *iResource.m_context);
    n_buffer::destroyBuffer(iResource.indexBuffer , *iResource.m_context);

    for (auto &buffer : iResource.uboBuffer)  n_buffer::destroyBuffer(buffer, *iResource.m_context);
    for (auto &buffer : iResource.ssboBuffer) n_buffer::destroyBuffer(buffer, *iResource.m_context);

    // Now safe to clear
    iResource.ubos.clear();
    iResource.ssbos.clear();
    iResource.uboBuffer.clear();
    iResource.ssboBuffer.clear();
  }
}
