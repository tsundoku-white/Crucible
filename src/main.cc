#include "src/vulkan/render.h"
#include <cmath>
#include <print>

int main()
{
  // init rendering code with vulkan
  IRenderer render{};

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
