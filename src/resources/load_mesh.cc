// load_mesh.cc
#include "load_mesh.h"
#include <cgltf.h>
#include <stdexcept>
#include <cstring>

namespace n_resource
{
  void loadMesh(MeshData &mesh, std::string path)
  {
    cgltf_options options{};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
      throw std::runtime_error("failed to parse glTF file: " + path);

    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
      cgltf_free(data);
      throw std::runtime_error("failed to load glTF buffers: " + path);
    }

    mesh.m_vertex_data.clear();
    mesh.m_index_data.clear();

    // Just take the first mesh / first primitive for now.
    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0)
    {
      cgltf_free(data);
      throw std::runtime_error("glTF file has no mesh primitives: " + path);
    }

    cgltf_primitive &prim = data->meshes[0].primitives[0];

    cgltf_accessor *posAccessor = nullptr;
    cgltf_accessor *normalAccessor = nullptr;
    cgltf_accessor *uvAccessor = nullptr;

    for (size_t i = 0; i < prim.attributes_count; i++)
    {
      cgltf_attribute &attr = prim.attributes[i];
      if (attr.type == cgltf_attribute_type_position) posAccessor = attr.data;
      else if (attr.type == cgltf_attribute_type_normal) normalAccessor = attr.data;
      else if (attr.type == cgltf_attribute_type_texcoord) uvAccessor = attr.data;
    }

    if (!posAccessor)
    {
      cgltf_free(data);
      throw std::runtime_error("glTF primitive has no POSITION attribute: " + path);
    }

    size_t vertexCount = posAccessor->count;
    mesh.m_vertex_data.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; i++)
    {
      Vertex &v = mesh.m_vertex_data[i];

      cgltf_accessor_read_float(posAccessor, i, &v.m_pos.x, 3);

      if (normalAccessor)
        cgltf_accessor_read_float(normalAccessor, i, &v.m_normal.x, 3);
      else
        v.m_normal = glm::vec3(0.0f, 0.0f, 0.0f);

      if (uvAccessor)
        cgltf_accessor_read_float(uvAccessor, i, &v.m_uv.x, 2);
      else
        v.m_uv = glm::vec2(0.0f, 0.0f);
    }

    if (prim.indices)
    {
      size_t indexCount = prim.indices->count;
      mesh.m_index_data.resize(indexCount);
      for (size_t i = 0; i < indexCount; i++)
        mesh.m_index_data[i] = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i));
    }
    else
    {
      // No index buffer in the file — synthesize a trivial 0..N-1 index list.
      mesh.m_index_data.resize(vertexCount);
      for (size_t i = 0; i < vertexCount; i++)
        mesh.m_index_data[i] = static_cast<uint32_t>(i);
    }

    mesh.m_mesh_path = path;

    cgltf_free(data);
  }
}
