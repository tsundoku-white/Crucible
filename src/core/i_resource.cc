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
    auto &allEntities = registery.getEntityView();
    uint32_t loadProg = 0;
    for (size_t e = 0; e < allEntities.size(); e++)
    {
      for (auto &model : iResource.m_registery->viewMutable<Model>())
      {
        // Create vertex and index buffers for this model
        if (!iResource.m_isBufferCreated)
        {
          n_buffer::createVertexBuffer(iResource.vertexBuffer, iResource.m_command, context,
              model.m_mesh_meta_data.m_vertex_data);
          n_buffer::createIndexBuffer(iResource.indexBuffer, iResource.m_command, context,
              model.m_mesh_meta_data.m_index_data);
          iResource.m_isBufferCreated = true;
        }
      }

      iResource.ubos.resize(allEntities.size());
      iResource.ssbos.resize(allEntities.size());

      size_t buffer_slots = allEntities.size() > 0 ? allEntities.size() : 1;
      n_buffer::createUniformBuffer(iResource.uboBuffer, context,
          sizeof(UniformBufferObject) * buffer_slots);

      n_buffer::createStorageBuffer(iResource.ssboBuffer, context,
          sizeof(glm::mat4) * buffer_slots);

      n_descriptor::createDescriptorSets(iResource.m_descriptor, context, iRender.m_pipeline.m_descriptorLayout,
          iResource.uboBuffer, iResource.ssboBuffer, iRender.m_maxFramesInFlight);

      loadProg++;
      printProgressBar("Updating cache", loadProg, allEntities.size());
    }
  }

  void transformUpdate(IResource &iResource)
  {
    if (!iResource.m_dirty_transform) return;

    for (auto &transform : iResource.m_registery->viewMutable<Transform>())
    {
      transform.m_forward = transform.m_rotation * glm::vec3( 0,  0, -1);
      transform.m_up      = transform.m_rotation * glm::vec3( 0,  1,  0);
      transform.m_right   = transform.m_rotation * glm::vec3( 1,  0,  0);
    }
  }

  void cameraUpdate(IResource &iResource, IRender &iRender)
  {
    if (!iResource.m_dirty_camera) return;

    auto &cameras = iResource.m_registery->viewMutable<Camera>();
    for (size_t i = 0; i < cameras.size(); i++)
    {
      EntityID id = iResource.m_registery->entityAt<Camera>(i);
      if (!iResource.m_registery->has<Transform>(id)) continue;

      auto &camera    = cameras[i];
      auto &transform = iResource.m_registery->get<Transform>(id);

      if (iResource.ubos.size() <= id) iResource.ubos.resize(id + 1);

      if (iResource.m_dirty_projection)

      {
        float aspect = static_cast<float>(iRender.m_context->m_swapchain_extent.width) / 
          static_cast<float>(iRender.m_context->m_swapchain_extent.height);

        iResource.ubos[id].m_projectionMatrix = glm::perspective(
            glm::radians(camera.m_fov),
            aspect,
            camera.m_minViewDistance,
            camera.m_maxViewDistance
            );

        iResource.ubos[id].m_projectionMatrix[1][1] *= -1;
        std::print("updated camera projection\n");
        iResource.m_dirty_projection = false;
      }
      iResource.ubos[id].m_viewMatrix = glm::lookAt(
          transform.m_location,
          transform.m_forward + transform.m_location,
          transform.m_up
          );
    }
  }

  void modelUpdate(IResource &iResource)
  {
    if (!iResource.m_dirty_model) return;
    auto &models = iResource.m_registery->viewMutable<Model>();
    for (size_t i = 0; i < models.size(); i++)
    {
      EntityID id = iResource.m_registery->entityAt<Model>(i);
      if (!iResource.m_registery->has<Transform>(id)) continue;

      auto &transform = iResource.m_registery->get<Transform>(id);

      glm::mat4 matrix(1.f);
      matrix = glm::translate(matrix, transform.m_location);
      matrix = matrix * glm::toMat4(transform.m_rotation);
      matrix = glm::scale(matrix, transform.m_scale);
      iResource.ssbos[id].m_modelsMatrix = matrix;
    }
  }

  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender)
  {
    auto &all_entities = registery.getEntityView();

    iResource.m_context   = &context;
    iResource.m_registery = &registery;

    n_command::createCommand(iResource.m_command, context, iRender);
    updateCache(iResource, registery, context, iRender);

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

    transformUpdate(iResource);
    cameraUpdate(iResource, iRender);
    modelUpdate(iResource);

      for (size_t e = 0; e < allEntities.size(); e++)
      {
        EntityID id = allEntities[e];
        modelMatrices.push_back(iResource.ssbos[id].m_modelsMatrix); // if that's the intent
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

