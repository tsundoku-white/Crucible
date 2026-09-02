#include "src/core/i_render.h"
#include <src/vulkan/window.h>
#include <src/vulkan/context.h>

int main()
{
  Window  window;
  Context context;
  IRender render;

  n_window::createWindow(window);
  n_context::createContext(context, window);
  n_render::createIRender(render, context, window);
  
  while (n_window::shouldClose(window))
  {
    n_window::pollEvents();
    n_render::draw_IRender(render);
  }

  n_render::destoryIRender(render);
  n_context::destoryContext(context);
  n_window::destoryWindow(window);
}
