#include "src/core/i_render.h"
#include "src/core/i_resource.h"
#include "src/vulkan/command.h"
#include "src/vulkan/context.h"
#include "src/vulkan/window.h"
#include <iostream>
#include <print>
#include <src/ecs/registery.h>
#include "src/ecs/components/transform.h"
#include "src/ecs/components/camera.h"
#include "src/ecs/components/model.h"
#include "src/core/i_input.h"

int main()
{
  Window window{};
  n_window::createWindow(window);

  Context context{};
  n_context::createContext(context, window);

  IRender iRender{};
  n_render::createIRender(iRender, context, window);

  Registery registery;

  Entity freeCam = registery.createEntity();
  Transform camTransform{};
  camTransform.m_location = glm::vec3(0, 0, 3);
  registery.add<Transform>(freeCam.getId(), camTransform);
  registery.add<Camera   >(freeCam.getId(), Camera{}   );

  Entity box0 = registery.createEntity();
  registery.add<Transform>(box0.getId(), Transform{});

  Model model{.m_path = MODEL_PATH "cube.glb"};
  n_resource::loadMesh(model.m_mesh_meta_data, model.m_path); 
  registery.add<Model>(box0.getId(), model);

  Entity box1 = registery.createEntity();
  registery.add<Transform>(box1.getId(), Transform{.m_location = {3,0,0}});

  Model model1{.m_path = MODEL_PATH "cube.glb"};
  n_resource::loadMesh(model1.m_mesh_meta_data, model1.m_path); 
  registery.add<Model>(box1.getId(), model);

  Entity box2 = registery.createEntity();
  registery.add<Transform>(box2.getId(), Transform{.m_location = {-3,0,0}});

  Model model2{.m_path = MODEL_PATH "cube.glb"};
  n_resource::loadMesh(model2.m_mesh_meta_data, model2.m_path); 
  registery.add<Model>(box2.getId(), model);

  IResource iResource{};
  n_resource::createResource(iResource, registery, context, iRender);

  Input input;
  n_input::create(input, window);

  float moveSpeed = 8.0f;
  float mouseSensitivity = 0.1f;
  static float yaw   = 0.0f;
  static float pitch = 0.0f;

  while (!n_window::shouldClose(window))
  {
    float moveAmount = moveSpeed * iRender.m_deltaTime;

    Transform &transform = *iResource.m_transform_cache[freeCam.getId()];

    glm::vec2 delta = n_input::mouse_delta(input);

    yaw   -= delta.x * mouseSensitivity;
    pitch += delta.y * mouseSensitivity;
    pitch  = glm::clamp(pitch, -89.0f, 89.0f);

    glm::quat qYaw   = glm::angleAxis(glm::radians(yaw),   glm::vec3(0, 1, 0));
    glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0));
    transform.m_rotation = glm::normalize(qYaw * qPitch);

    iResource.m_dirty_transform = true;
    iResource.m_dirty_camera    = true;

    if (n_input::is_pressed(input, KEY_A)) transform.m_location -= transform.m_right   * moveAmount;
    if (n_input::is_pressed(input, KEY_D)) transform.m_location += transform.m_right   * moveAmount;
    if (n_input::is_pressed(input, KEY_W)) transform.m_location += transform.m_forward * moveAmount;
    if (n_input::is_pressed(input, KEY_S)) transform.m_location -= transform.m_forward * moveAmount;
    if (n_input::is_pressed(input, KEY_SPACE))      transform.m_location.y += moveAmount;
    if (n_input::is_pressed(input, KEY_LEFT_SHIFT)) transform.m_location.y -= moveAmount;

    n_window::pollEvents();
    n_render::drawIRender(iRender, iResource);
  }
  vkDeviceWaitIdle(context.m_device);
  n_render::destoryIRender(iRender);
  n_resource::destroyResource(iResource);
  n_context::destroyContext(context);
  n_window::destroyWindow(window);

}
