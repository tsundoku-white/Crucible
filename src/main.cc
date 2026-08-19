#include "src/vulkan/render.h"
#include "src/ecs/ecs.h"
#include <print>

struct Transform 
{
  float x,y;
};

int main()
{
  // init rendering code with vulkan
  IRenderer render{};

  ECS ecs;

  Entity e = ecs.create_entity();
  ecs.add_component<Transform>(e, Transform {.x = 10, .y = 20} );

  float time = 0.f;
  while (!render.should_close())
  {
    render.draw();

    time += render.delta_time;
    if (time >= 0.5)
    {
      std::print("fps: {}\n", 1 / render.delta_time);
      time = 0;
    }
  }
}
