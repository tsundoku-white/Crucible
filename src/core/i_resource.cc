#include "src/core/i_resource.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <print>
#include <src/core/i_render.h>
#include <src/core/pch.h>
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
    // get all entities.
    auto &allEntities = registery.getEntityView();
    uint32_t loadProg = 0;

    // loop through all entities
    for (size_t e = 0; e < allEntities.size(); e++)
    {
      // loop through all model components
      for (auto &model : iResource.m_registery->viewMutable<Model>())
      {
        if (!iResource.m_isBufferCreated)
        {
          // vertex/index buffer creation if buffers not created yet. 
          n_buffer::createVertexBuffer(iResource.vertexBuffer, iResource.m_command, context,
              model.m_mesh_meta_data.m_vertex_data);
          n_buffer::createIndexBuffer(iResource.indexBuffer, iResource.m_command, context,
              model.m_mesh_meta_data.m_index_data);
          n_image::createTexture(iResource.m_texture, context, iResource.m_command,
      TEXTURE_PATH "proto.png");
          iResource.m_isBufferCreated = true;
        }
      }
      loadProg++;
      printProgressBar("Updating cache", loadProg, allEntities.size());
    }

    // resize ubo and ssbo to all entities.
    iResource.ubos.resize(allEntities.size());
    iResource.ssbos.resize(allEntities.size());

    // create a var the checks if all entities is grater than 0 if its not set to 1.
    size_t buffer_slots = allEntities.size() > 0 ? allEntities.size() : 1;
    n_buffer::createUniformBuffer(iResource.uboBuffer, context,
        sizeof(UniformBufferObject) * buffer_slots);

    n_buffer::createStorageBuffer(iResource.ssboBuffer, context,
        sizeof(glm::mat4) * buffer_slots);

    // create descriptor to set to data acan be pushed to shaderes
    n_descriptor::createDescriptorSets(
        iResource.m_descriptor,
        context,
        iResource.uboBuffer,
        iResource.ssboBuffer,
        iResource.m_texture.view,
        iRender.m_maxFramesInFlight
        );
  } 

  void transformUpdate(IResource &iResource)
  {
    // if transform is not dirty then skip.
    if (!iResource.m_dirty_transform) return;

    // cycle through all of the transform components and updating 
    // m_forward/m_up/m_right based on m_rotation.
    for (auto &transform : iResource.m_registery->viewMutable<Transform>())
    {
      transform.m_forward = transform.m_rotation * glm::vec3( 0,  0, -1);
      transform.m_up      = transform.m_rotation * glm::vec3( 0,  1,  0);
      transform.m_right   = transform.m_rotation * glm::vec3( 1,  0,  0);
    }
  }

  void cameraUpdate(IResource &iResource, IRender &iRender)
  {
    // if camera is not dirty then skip.
    if (!iResource.m_dirty_camera && !iResource.m_dirty_transform) return;

    // cycle through alll camera components
    auto &cameras = iResource.m_registery->viewMutable<Camera>();
    for (size_t i = 0; i < cameras.size(); i++)
    {
      // check is current camera has a transform because transform is a dependnecy
      EntityID id = iResource.m_registery->entityAt<Camera>(i);
      if (!iResource.m_registery->has<Transform>(id)) continue;

      // create local var for readability.
      auto &camera    = cameras[i];
      auto &transform = iResource.m_registery->get<Transform>(id);

      // make sure the the ubo size is leess or equal to id + 1.
      if (iResource.ubos.size() <= id) iResource.ubos.resize(id + 1);

      // check if projection need to be updated.
      if (iResource.m_dirty_projection)
      {
        // aspect is width/height of the screen which is stored in m_swapchain_extent also 
        // the canvus size.
        float aspect = static_cast<float>(iRender.m_context->m_swapchain_extent.width) / 
          static_cast<float>(iRender.m_context->m_swapchain_extent.height);

        // updating the projection matrix with new fov near and far exe.
        iResource.ubos[id].m_projectionMatrix = glm::perspective(
            glm::radians(camera.m_fov),
            aspect,
            camera.m_minViewDistance,
            camera.m_maxViewDistance
            );

        // fliping projection because vulkan renders it upside down.
        iResource.ubos[id].m_projectionMatrix[1][1] *= -1;
        std::print("updated camera projection\n");

        // make projection not dirty.
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
    // if models not dirty skip.
    if (!iResource.m_dirty_model) return;

    // get local models.
    auto &models = iResource.m_registery->viewMutable<Model>();

    // cycle through aal models.
    for (size_t i = 0; i < models.size(); i++)
    {
      // get current id and check if it has a transform because it is a dependnecy.
      EntityID id = iResource.m_registery->entityAt<Model>(i);
      if (!iResource.m_registery->has<Transform>(id)) continue;

      auto &transform = iResource.m_registery->get<Transform>(id);

      // create a local temporery matrix.
      glm::mat4 matrix(1.f);
      // set the location, rotation, scale and update the model's projection on screen. 
      matrix = glm::translate(matrix, transform.m_location);
      matrix = matrix * glm::toMat4(transform.m_rotation);
      matrix = glm::scale(matrix, transform.m_scale);

      // push to actule ssbo 
      iResource.ssbos[id].m_modelsMatrix = matrix;
    }
  }

  void createResource(IResource &iResource, Registery &registery, Context &context, IRender &iRender)
  {
    // get all entities
    auto &all_entities = registery.getEntityView();

    // copy the context & registery to iResource as a local copy.
    iResource.m_context   = &context;
    iResource.m_registery = &registery;

    // create the command buffer and updating the cache.
    n_descriptor::createDescriptorSetLayout(iResource.m_descriptor, context);
    n_pipeline::createPipeline(iResource.m_pipeline, iResource.m_descriptor, context);
    n_command::createCommand(iResource.m_command, context, iRender);
    updateCache(iResource, registery, context, iRender);

    // set all of the dirty check to true.
    iResource.m_dirty_transform   = true;
    iResource.m_dirty_projection  = true;
    iResource.m_dirty_camera      = true;
    iResource.m_dirty_model       = true;
  }

  void renderResourceUpdate(IResource &iResource, IRender &iRender)
  {
    // update all of the compontes in render loop
    transformUpdate(iResource);
    cameraUpdate(iResource, iRender);
    modelUpdate(iResource);

    // get all models and resizing the model matrices array
    auto &models = iResource.m_registery->viewMutable<Model>();
    iResource.m_modelMatrices.clear();
    iResource.m_modelMatrices.reserve(models.size());

    // push all of the matrices to in ssbo based on id aka. batching.
    for (size_t i = 0; i < models.size(); i++)
    {
      EntityID id = iResource.m_registery->entityAt<Model>(i);
      iResource.m_modelMatrices.push_back(iResource.ssbos[id].m_modelsMatrix);
    }

    // base the instance count based in size.
    iResource.m_modelInstanceCount = static_cast<uint32_t>(iResource.m_modelMatrices.size());

    // updated cache if need down the line.
    if (iResource.m_dirty_cache)
    {
      updateCache(iResource, *iResource.m_registery, *iResource.m_context, iRender);
    }

    // if ubo is not empty upload date to gpu
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

    // if model matrixes is not empty upload date to gpu
    if (!iResource.m_modelMatrices.empty())
    {
      void *data;
      vmaMapMemory(iRender.m_context->m_allocator,
          iResource.ssboBuffer.m_allocation, &data);
      memcpy(data, iResource.m_modelMatrices.data(),
          sizeof(glm::mat4) * iResource.m_modelMatrices.size());
      vmaUnmapMemory(iRender.m_context->m_allocator,
          iResource.ssboBuffer.m_allocation);
    }

    // reset dirty if need to. 
    if (iResource.m_dirty_cache     ) iResource.m_dirty_cache     = false;
    if (iResource.m_dirty_transform ) iResource.m_dirty_transform = false;
    if (iResource.m_dirty_camera    ) iResource.m_dirty_camera    = false;
    if (iResource.m_dirty_model     ) iResource.m_dirty_model     = false;
  }

  void destroyResource(IResource &iResource)
  {
    // Clear command and descriptor.
    n_command::destroyCommand(iResource.m_command , *iResource.m_context);
    n_pipeline::destoryPipeline(iResource.m_pipeline, *iResource.m_context);
    n_descriptor::destoryDescriptor(iResource.m_descriptor, *iResource.m_context);

    // clear buffers.
    n_buffer::destroyBuffer(iResource.vertexBuffer, *iResource.m_context);
    n_buffer::destroyBuffer(iResource.indexBuffer , *iResource.m_context);
    n_buffer::destroyBuffer(iResource.uboBuffer,  *iResource.m_context);
    n_buffer::destroyBuffer(iResource.ssboBuffer, *iResource.m_context);

    n_image::destroyTexture(iResource.m_texture, *iResource.m_context);
    // clear ubo/ssbo.
    iResource.ubos.clear();
    iResource.ssbos.clear();
  }
}
