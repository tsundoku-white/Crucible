#pragma once

enum ProjectionMode 
{
  PRESPECTIVE,
  ORTHOGRPHIC
};

struct Camera 
{
  float m_fov               = 90.f;
  float m_maxViewDistance   = 999.f;
  float m_minViewDistance   = .001f;
  bool  m_isPrimary         = false;
  ProjectionMode m_projMode = PRESPECTIVE;
};
