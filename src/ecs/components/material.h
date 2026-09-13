#pragma once

#include <src/vulkan/image.h>
struct Material 
{
  std::string m_base_texture_path       = "";
  std::string m_roughness_texture_path  = "";
  std::string m_metalic_texture_path    = "";
  std::string m_normal_texture_path     = "";

  Texture m_base      {};
  Texture m_roughness {};
  Texture m_metalic   {};
  Texture m_normal    {};
};
