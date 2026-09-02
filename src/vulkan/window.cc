#include "window.h"
#include <GLFW/glfw3.h>

namespace n_window
{
  void createWindow(Window &window)
  {
    if (!glfwInit())
    {
      throw std::runtime_error("failed to load glfw lib\n");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window.m_handle = glfwCreateWindow(window.m_width, window.m_height, window.m_name, nullptr, nullptr);
    if (!window.m_handle)
    {
      std::runtime_error("failed to create window\n");
    }

  }

  void destoryWindow(Window &window)              { glfwTerminate();  }
  void pollEvents()                               { glfwPollEvents(); }
  void setShouldClose(Window &window, bool value) { glfwSetWindowShouldClose(window.m_handle, value); }
  bool shouldClose(Window &window)                { return glfwWindowShouldClose(window.m_handle);    }
}
