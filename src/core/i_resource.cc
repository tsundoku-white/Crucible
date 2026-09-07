#include "src/core/i_resource.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <print>
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

  void updateCache(IResource &iResource, Registery &registery, Context &context, IRender &iRender)
  {
    auto &all_entities = registery.getEntityView();
    uint32_t loadProg = 0;
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
      loadProg++;
      printProgressBar("Updating cache", loadProg, all_entities.size());
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
        // Make sure ubos vector has a slot for this entity's id
        if (iResource.ubos.size() <= id)
          iResource.ubos.resize(id + 1);

        if (iResource.m_dirty_projection)
        {
          float aspect = static_cast<float>(iRender.m_context->m_swapchain_extent.width) / 
            static_cast<float>(iRender.m_context->m_swapchain_extent.height);

          iResource.ubos[id].m_projectionMatrix = glm::perspective(
              glm::radians(camera->m_fov),
              aspect,
              camera->m_minViewDistance,
              camera->m_maxViewDistance
              );

          iResource.ubos[id].m_projectionMatrix[1][1] *= -1;
          std::print("updated camera projection\n");
          iResource.m_dirty_projection = false;
        }
        iResource.ubos[id].m_viewMatrix = glm::lookAt(
            transform->m_location,
            transform->m_forward + transform->m_location,
            transform->m_up
            );
      }
    }
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
        if (iResource.ssbos.size() <= id)
          iResource.ssbos.resize(id + 1);

        glm::mat4 matrix(1.f);

        matrix = glm::translate(matrix, transform->m_location);
        matrix = matrix * glm::toMat4(transform->m_rotation);
        matrix = glm::scale(matrix, transform->m_scale);

        iResource.ssbos[id].m_modelsMatrix = matrix;
      }
    }
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

    uint32_t loadProg = 0;
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
        if (!iResource.m_isBufferCreated)
        {
          n_buffer::createVertexBuffer(iResource.vertexBuffer, iResource.m_command, context,
              model->m_mesh_meta_data.m_vertex_data);
          n_buffer::createIndexBuffer(iResource.indexBuffer, iResource.m_command, context,
              model->m_mesh_meta_data.m_index_data);
          iResource.m_isBufferCreated = true;
        }
      }

      if (has_transform)
        iResource.m_transform_cache[id] = &registery.get<Transform>(id);

      if (has_camera)
        iResource.m_camera_cache[id] = &registery.get<Camera>(id);

      loadProg++;
      printProgressBar("Loading resources", loadProg, all_entities.size());
    }

    size_t entity_count = all_entities.size();
    iResource.ubos.resize(entity_count);
    iResource.ssbos.resize(entity_count);

    size_t buffer_slots = entity_count > 0 ? entity_count : 1;
    n_buffer::createUniformBuffer(iResource.uboBuffer, context,
        sizeof(UniformBufferObject) * buffer_slots);
    n_buffer::createStorageBuffer(iResource.ssboBuffer, context,
        sizeof(glm::mat4) * buffer_slots);

    n_descriptor::createDescriptorSets(iResource.m_descriptor, context, iRender.m_pipeline.m_descriptorLayout,
        iResource.uboBuffer, iResource.ssboBuffer, iRender.m_maxFramesInFlight);

    iResource.m_dirty_transform   = true;
    iResource.m_dirty_projection  = true;
    iResource.m_dirty_camera      = true;
    iResource.m_dirty_model       = true;
  }

  void renderResourceUpdate(IResource &iResource, IRender &iRender)
  {

    auto &allEntities = iResource.m_registery->getEntityView();

    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(allEntities.size());

    for (size_t e = 0; e < allEntities.size(); e++)
    {
      EntityID id = allEntities[e];

      transformUpdate(iResource, id);
      cameraUpdate(iResource, iRender, id);
      modelUpdate(iResource, id);

      if (iResource.m_model_cache.find(id) != iResource.m_model_cache.end())
        modelMatrices.push_back(iResource.ssbos[id].m_modelsMatrix);
    }

    iResource.m_modelInstanceCount = static_cast<uint32_t>(modelMatrices.size());

    iResource.m_dirty_transform = false;
    iResource.m_dirty_camera    = false;
    iResource.m_dirty_model     = false;

    if (!iResource.ubos.empty())
    {
      void *data;
      vmaMapMemory(iRender.m_context->m_allocator,
          iResource.uboBuffer.m_allocation, &data);
      memcpy(data, iResource.ubos.data(),
          sizeof(UniformBufferObject) * iResource.ubos.size());
      vmaUnmapMemory(iRender.m_context->m_allocator,
          iResource.uboBuffer.m_allocation);
    }

    if (!modelMatrices.empty())
    {
      void *data;
      vmaMapMemory(iRender.m_context->m_allocator,
          iResource.ssboBuffer.m_allocation, &data);
      memcpy(data, modelMatrices.data(),
          sizeof(glm::mat4) * modelMatrices.size());
      vmaUnmapMemory(iRender.m_context->m_allocator,
          iResource.ssboBuffer.m_allocation);
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

    n_buffer::destroyBuffer(iResource.uboBuffer,  *iResource.m_context);
    n_buffer::destroyBuffer(iResource.ssboBuffer, *iResource.m_context);

    iResource.ubos.clear();
    iResource.ssbos.clear();
  }
}
