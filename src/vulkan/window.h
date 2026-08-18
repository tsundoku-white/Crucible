#pragma once
#include "src/pch.h"

struct GLFWwindow;

struct Vk_Window
{
  GLFWwindow *m_handle;
  uint32_t    m_width = 1270;
  uint32_t    m_height = 720;
  const char *m_title = "untitled"; 
  bool        m_is_window_resized = false;
};

namespace n_window 
{
  bool should_close(Vk_Window &window);
  void create(Vk_Window &window);
  void destroy(Vk_Window &window);
}
