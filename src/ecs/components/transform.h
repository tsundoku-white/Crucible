#pragma once

struct Transform 
{
  glm::vec3 m_location;
  glm::quat m_rotation;
  glm::vec3 m_scale;

  glm::vec3 m_forward;
  glm::vec3 m_right;
  glm::vec3 m_up;
};
