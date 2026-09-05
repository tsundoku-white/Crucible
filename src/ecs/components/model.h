#pragma once

#include "src/core/pch.h"
#include "src/resources/load_mesh.h"

struct Model 
{
  std::string m_path            = MODEL_PATH "cube.glb";
  MeshData    m_mesh_meta_data  = {};
};
