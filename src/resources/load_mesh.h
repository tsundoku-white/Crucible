#pragma once

#include "src/core/pch.h"
#include "src/vulkan/descriptor.h"

struct MeshData
{
  std::string           m_mesh_path   = MODEL_PATH "default.glb";
  std::vector<Vertex>   m_vertex_data = {};
  std::vector<uint32_t> m_index_data  = {};
};

namespace n_resource
{
  void load_mesh(MeshData &mesh, std::string path);
  void destory_pipeline(MeshData &mesh);
}
