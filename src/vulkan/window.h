#pragma once

#include "src/core/pch.h"

struct GLFWwindow;

struct Window
{
  GLFWwindow *m_handle    = nullptr;
  uint32_t    m_width     = 1270;
  uint32_t    m_height    = 720;
  const char *m_name      = "Crucible";
  bool        m_isResized = false;
};

namespace n_window
{
  void createWindow(Window &window);
  void destoryWindow(Window &window);

  void pollEvents();

  void setShouldClose(Window &window, bool value);
  bool shouldClose(Window &window);
}
