#include "src/core/i_resource.h"
#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <print>
#include <src/core/i_render.h>
#include <src/core/pch.h>
#include <src/ecs/components/material.h>
#include <src/ecs/entity.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/image.h>
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

    // ---- Tear down previous GPU resources so we don't leak ----
    if (iResource.m_isBufferCreated)
    {
      n_descriptor::destoryDescriptor(iResource.m_descriptor, context);
      n_buffer::destroyBuffer(iResource.vertexBuffer, context);
      n_buffer::destroyBuffer(iResource.indexBuffer,  context);
      n_buffer::destroyBuffer(iResource.uboBuffer,    context);
      n_buffer::destroyBuffer(iResource.ssboBuffer,   context);

      for (auto &t : iResource.m_textures)
        n_image::destroyTexture(t, context);
      iResource.m_textures.clear();
    }

    // ---- Vertex / index buffers (once, from first model) ----
    for (auto &model : registery.viewMutable<Model>())
    {
      n_buffer::createVertexBuffer(iResource.vertexBuffer, iResource.m_command, context,
          model.m_mesh_meta_data.m_vertex_data);
      n_buffer::createIndexBuffer(iResource.indexBuffer, iResource.m_command, context,
          model.m_mesh_meta_data.m_index_data);
      break;
    }

    // ---- Textures (once, one per material) ----
    iResource.m_materialTextureIndex.clear();

    for (size_t i = 0; i < registery.viewMutable<Material>().size(); i++)
    {
      // get the id ref and make local ref object 
      EntityID id = registery.entityAt<Material>(i);
      auto &material = registery.viewMutable<Material>()[i];

      // create a texture
      n_image::createTexture(material.m_base, context,
          iResource.m_command, material.m_base_texture_path);

      // check that the textures dont go out of bounds if it does resize to the max of u32
      if (iResource.m_materialTextureIndex.size() <= id)
        iResource.m_materialTextureIndex.resize(id + 1, UINT32_MAX);

      iResource.m_materialTextureIndex[id] = static_cast<uint32_t>(iResource.m_textures.size());
      iResource.m_textures.push_back(material.m_base);
    }

    // ---- UBO / SSBO ----
    size_t buffer_slots = allEntities.size() > 0 ? allEntities.size() : 1;

    iResource.ubos.resize(allEntities.size());
    iResource.ssbos.resize(allEntities.size());

    // create size on gpu for both ubo and ssbo with proper amount of slots
    n_buffer::createUniformBuffer(iResource.uboBuffer, context,
        sizeof(UniformBufferObject) * buffer_slots);

    n_buffer::createStorageBuffer(iResource.ssboBuffer, context,
        sizeof(ShaderStorageBufferObject) * buffer_slots);

    // ---- Descriptor sets ----
    n_descriptor::createDescriptorSets(
        iResource.m_descriptor,
        context,
        iResource.uboBuffer,
        iResource.ssboBuffer,
        iResource.m_textures,
        iRender.m_maxFramesInFlight);

    iResource.m_isBufferCreated = true;
  }

  void transformUpdate(IResource &iResource)
  {
    if (!iResource.m_dirty_transform) return;

    // calc the new directions based on the rotation.
    for (auto &transform : iResource.m_registery->viewMutable<Transform>())
    {
      transform.m_forward = transform.m_rotation * glm::vec3( 0,  0, -1);
      transform.m_up      = transform.m_rotation * glm::vec3( 0,  1,  0);
      transform.m_right   = transform.m_rotation * glm::vec3( 1,  0,  0);
    }
  }

  void cameraUpdate(IResource &iResource, IRender &iRender)
  {
    if (!iResource.m_dirty_camera && !iResource.m_dirty_transform) return;

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
            glm::radians(camera.m_fov), aspect,
            camera.m_minViewDistance, camera.m_maxViewDistance);
        iResource.ubos[id].m_projectionMatrix[1][1] *= -1;
        std::print("updated camera projection\n");
        iResource.m_dirty_projection = false;
      }

      iResource.ubos[id].m_viewMatrix = glm::lookAt(
          transform.m_location,
          transform.m_forward + transform.m_location,
          transform.m_up);
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
      uint32_t texIndex = (id < iResource.m_materialTextureIndex.size())
        ? iResource.m_materialTextureIndex[id]
        : UINT32_MAX;

      iResource.ssbos[id].m_texture_index = (texIndex != UINT32_MAX) ? texIndex : 0;
    }
  }

  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender)
  {
    iResource.m_context   = &context;
    iResource.m_registery = &registery;

    n_descriptor::createDescriptorSetLayout(iResource.m_descriptor, context);
    n_pipeline::createPipeline(iResource.m_pipeline, iResource.m_descriptor, context);
    n_command::createCommand(iResource.m_command, context, iRender);
    updateCache(iResource, registery, context, iRender);

    iResource.m_dirty_transform   = true;
    iResource.m_dirty_projection  = true;
    iResource.m_dirty_camera      = true;
    iResource.m_dirty_model       = true;
  }

  void renderResourceUpdate(IResource &iResource, IRender &iRender)
  {
    // update all of the components.
    transformUpdate(iResource);
    cameraUpdate(iResource, iRender);
    modelUpdate(iResource);

    // get a ref to all models and clear and reserve size of all of models
    auto &models = iResource.m_registery->viewMutable<Model>();
    iResource.m_instances.clear();
    iResource.m_instances.reserve(models.size());

    // push all of the models in instances vector
    for (size_t i = 0; i < models.size(); i++)
    {
      EntityID id = iResource.m_registery->entityAt<Model>(i);
      iResource.m_instances.push_back(iResource.ssbos[id]);
    }

    // set the instance count via the size of instances as a u32
    iResource.m_modelInstanceCount = static_cast<uint32_t>(iResource.m_instances.size());

    if (iResource.m_dirty_cache)
      updateCache(iResource, *iResource.m_registery, *iResource.m_context, iRender);

    // ubo update
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

    // ssbo update
    if (!iResource.m_instances.empty())
    {
      void *data;
      vmaMapMemory(iRender.m_context->m_allocator,
          iResource.ssboBuffer.m_allocation, &data);
      memcpy(data, iResource.m_instances.data(),
          sizeof(ShaderStorageBufferObject) * iResource.m_instances.size());
      vmaUnmapMemory(iRender.m_context->m_allocator,
          iResource.ssboBuffer.m_allocation);
    }

    if (iResource.m_dirty_cache    ) iResource.m_dirty_cache     = false;
    if (iResource.m_dirty_transform) iResource.m_dirty_transform = false;
    if (iResource.m_dirty_camera   ) iResource.m_dirty_camera    = false;
    if (iResource.m_dirty_model    ) iResource.m_dirty_model     = false;
  }

  void destroyResource(IResource &iResource)
  {
    n_command::destroyCommand(iResource.m_command, *iResource.m_context);
    n_pipeline::destoryPipeline(iResource.m_pipeline, *iResource.m_context);
    n_descriptor::destoryDescriptor(iResource.m_descriptor, *iResource.m_context);

    n_buffer::destroyBuffer(iResource.vertexBuffer, *iResource.m_context);
    n_buffer::destroyBuffer(iResource.indexBuffer , *iResource.m_context);
    n_buffer::destroyBuffer(iResource.uboBuffer,   *iResource.m_context);
    n_buffer::destroyBuffer(iResource.ssboBuffer,  *iResource.m_context);

    for (auto &texture : iResource.m_textures)
      n_image::destroyTexture(texture, *iResource.m_context);

    iResource.m_textures.clear();
    iResource.ubos.clear();
    iResource.ssbos.clear();
    iResource.m_instances.clear();
  }
}
