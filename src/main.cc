#include "src/core/i_render.h"
#include "src/core/i_resource.h"
#include "src/vulkan/command.h"
#include "src/vulkan/context.h"
#include "src/vulkan/window.h"
#include <iostream>
#include <src/ecs/registery.h>
#include "src/ecs/components/transform.h"
#include "src/ecs/components/camera.h"
#include "src/ecs/components/model.h"

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
  registery.add<Transform>(freeCam.getId(), Transform{});
  registery.add<Camera   >(freeCam.getId(), Camera{}   );

  Entity box0 = registery.createEntity();
  registery.add<Transform>(box0.getId(), Transform{});

  Model model{.m_path = MODEL_PATH "cube.glb"};
  n_resource::loadMesh(model.m_mesh_meta_data, model.m_path); 
  registery.add<Model>(box0.getId(), model);

  IResource iResource{};
  n_resource::createResource(iResource, registery, context, iRender);

  while (!n_window::shouldClose(window))
  {
    n_window::pollEvents(); 
    n_resource::renderResourceUpdate(iResource, iRender);
    n_render::drawIRender(iRender, iResource);
  }

  vkDeviceWaitIdle(context.m_device);
  n_render::destoryIRender(iRender);
  n_resource::destroyResource(iResource);
  n_context::destroyContext(context);
  n_window::destroyWindow(window);

}
