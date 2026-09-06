#include "window.h"
#include <GLFW/glfw3.h>

namespace n_window
{
  void framebuffer_size_callback(GLFWwindow* window, int width, int height) 
{
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    
    if (win) 
    {
      win->m_height     = height;
      win->m_width      = width;
      win->m_isResized  = true;
    }
}
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
    glfwSetWindowUserPointer(window.m_handle, &window);
    glfwSetFramebufferSizeCallback(window.m_handle, framebuffer_size_callback);
  }

  void destroyWindow(Window &window)              { glfwTerminate();  }
  void pollEvents()                               { glfwPollEvents(); }
  void setShouldClose(Window &window, bool value) { glfwSetWindowShouldClose(window.m_handle, value); }
  bool shouldClose(Window &window)                { return glfwWindowShouldClose(window.m_handle);    }
}
