#include "window.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) 
{
    Vk_Window* win = static_cast<Vk_Window*>(glfwGetWindowUserPointer(window));
    
    if (win) 
    {
      win->m_is_window_resized = true;
    }
}

namespace n_window
{
  bool should_close(Vk_Window &window)
  {
    glfwPollEvents();
    return glfwWindowShouldClose(window.m_handle);
  }

  void create(Vk_Window &window)
  {
    if (!glfwInit())
      std::runtime_error("failed to load glfw\n");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window.m_handle = glfwCreateWindow(window.m_width, window.m_height, window.m_title, nullptr, nullptr);

    if (!window.m_handle)
    {
      std::runtime_error("failed to create window\n");
      glfwTerminate();
    }

    glfwSetWindowUserPointer(window.m_handle, &window);
    glfwSetFramebufferSizeCallback(window.m_handle, framebuffer_size_callback);

    std::print("\e[0;32m" "pass: " "\e[0m" "window\n");
  }

  void destroy(Vk_Window &window)
  {
    if (!window.m_handle) return;
    glfwDestroyWindow(window.m_handle);
    glfwTerminate();
  }
}
