#include "src/core/i_render.h"
#include "src/core/i_resource.h"
#include "src/vulkan/command.h"
#include "src/vulkan/context.h"
#include "src/vulkan/window.h"
#include <iostream>
#include <print>
#include <src/core/pch.h>
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
  camTransform.m_location = glm::vec3(0, 0, 20);
  registery.add<Transform>(freeCam.getId(), camTransform);
  registery.add<Camera   >(freeCam.getId(), Camera{}   );

  constexpr int   GRID_SIZE = 20;
  constexpr float GRID_SPACING = 2.5f;
  constexpr float GRID_OFFSET  = (GRID_SIZE - 1) * GRID_SPACING * 0.5f;

  for (int x = 0; x < GRID_SIZE; ++x)
  {
    for (int y = 0; y < GRID_SIZE; ++y)
    {
      for (int z = 0; z < GRID_SIZE; ++z)
      {
        Entity box = registery.createEntity();
        registery.add<Transform>(box.getId(), Transform{.m_location = {
          x * GRID_SPACING - GRID_OFFSET,
          y * GRID_SPACING - GRID_OFFSET,
          z * GRID_SPACING - GRID_OFFSET
        }});

        Model model{.m_path = MODEL_PATH "cube.glb"};
        n_resource::loadMesh(model.m_mesh_meta_data, model.m_path);
        registery.add<Model>(box.getId(), model);
      }
    }
  }

  IResource iResource{};
  n_resource::createResource(iResource, registery, context, iRender);

  Input input;
  n_input::create(input, window);

  float moveSpeed = 20.0f;
  float mouseSensitivity = 0.1f;
  static float yaw   = 0.0f;
  static float pitch = 0.0f;

  float time = 0;
  while (!n_window::shouldClose(window))
  {
    float moveAmount = moveSpeed * iRender.m_deltaTime;

    Transform &transform = registery.get<Transform>(freeCam.getId());

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

    // print debug
    time += g_frameStats.m_deltaTime;
    if (time > 1)
    {
    std::print(
        "delta time:    {:.5f}\n"
        "fps:           {:.2f}\n"
        "frame time ms: {:.2f}\n"
        "draw calls /s: {}\n", 
        g_frameStats.m_deltaTime,
        g_frameStats.m_fps,
        g_frameStats.m_frameTimeMs,
        g_frameStats.m_drawCalls);
    time = 0;
    }
  }
  vkDeviceWaitIdle(context.m_device);
  n_render::destoryIRender(iRender);
  n_resource::destroyResource(iResource);
  n_context::destroyContext(context);
  n_window::destroyWindow();
}
