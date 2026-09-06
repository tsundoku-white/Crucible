#pragma once

struct Transform 
{
  glm::vec3 m_location = glm::vec3(0,0,0);
  glm::quat m_rotation = glm::quat(1,0,0,0);
  glm::vec3 m_scale = glm::vec3(1,1,1);

  glm::vec3 m_forward = glm::vec3(0,0,-1);
  glm::vec3 m_right = glm::vec3(1,0,0);
  glm::vec3 m_up = glm::vec3(0,1,0);
};
