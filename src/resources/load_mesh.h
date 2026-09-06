#pragma once

#include "src/core/pch.h"
#include "src/vulkan/buffer.h"

struct MeshData
{
  std::vector<Vertex>   m_vertex_data = {};
  std::vector<uint32_t> m_index_data  = {};
};

namespace n_resource
{
  void loadMesh(MeshData &mesh, std::string path);
}
