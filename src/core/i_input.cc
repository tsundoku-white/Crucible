#include "i_input.h"
#include "GLFW/glfw3.h"
#include "glm/ext/vector_float2.hpp"
#include "src/vulkan/window.h"
#include <system_error>

namespace n_input
{
  void create(Input &input, Window &window)
  {
    input.m_window = &window;
    input.m_first_mouse = true;
    glfwSetInputMode(input.m_window->m_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  bool is_pressed(Input &input, KEY key)
  {
    return glfwGetKey(input.m_window->m_handle, key) == GLFW_PRESS;
  }

  glm::vec2 mouse_location(Input &input)
  {
    glfwGetCursorPos(input.m_window->m_handle, &input.m_last_x, &input.m_last_y);
    return glm::vec2(input.m_last_x, input.m_last_y);
  }

  glm::vec2 mouse_delta(Input &input)
  {
    double current_x, current_y;
    glfwGetCursorPos(input.m_window->m_handle, &current_x, &current_y);

    if (input.m_first_mouse)
    {
      input.m_last_x = current_x;
      input.m_last_y = current_y;
      input.m_first_mouse = false;
    }

    input.m_delta_x = current_x - input.m_last_x;
    input.m_delta_y = input.m_last_y - current_y; // Invert Y

    input.m_last_x = current_x;
    input.m_last_y = current_y;

    return glm::vec2(input.m_delta_x, input.m_delta_y);
  }
}
