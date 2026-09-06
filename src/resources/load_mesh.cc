#include "load_mesh.h"
#include <print>
#include <stdexcept>

#include "cgltf.h"

namespace n_resource
{
  void loadMesh(MeshData &mesh, std::string path)
  {
    cgltf_options options = {};
    cgltf_data*   data    = NULL;

    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success)
    {
      throw std::runtime_error("failed to load mesh");
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success)
    {
      cgltf_free(data);
      return;
    }

    for (size_t m = 0; m < data->meshes_count; m++)
    {
      cgltf_mesh *currentMesh = &data->meshes[m];

      for (size_t p = 0; p < currentMesh->primitives_count; p++)
      {
        cgltf_primitive *primitive = &currentMesh->primitives[p];

        cgltf_accessor *positionAccessor = nullptr;
        cgltf_accessor *normalAccessor   = nullptr;
        cgltf_accessor *uvAccessor       = nullptr;

        for (size_t a = 0; a < primitive->attributes_count; a++)
        {
          cgltf_attribute *attr = &primitive->attributes[a];

          switch (attr->type)
          {
            case cgltf_attribute_type_position:
              positionAccessor = attr->data;
              break;
            case cgltf_attribute_type_normal:
              normalAccessor = attr->data;
              break;
            case cgltf_attribute_type_texcoord:
              if (!uvAccessor)
                uvAccessor = attr->data;
              break;
            default:
              break;
          }
        }

        if (!positionAccessor)
          continue; // primitive has no positions, nothing we can build

        size_t vertexCount = positionAccessor->count;
        uint32_t baseVertex = static_cast<uint32_t>(mesh.m_vertex_data.size());

        for (size_t i = 0; i < vertexCount; i++)
        {
          Vertex vertex = {};

          cgltf_accessor_read_float(positionAccessor, i, &vertex.m_pos.x, 3);

          if (normalAccessor)
            cgltf_accessor_read_float(normalAccessor, i, &vertex.m_normal.x, 3);

          if (uvAccessor)
            cgltf_accessor_read_float(uvAccessor, i, &vertex.m_uv.x, 2);

          mesh.m_vertex_data.push_back(vertex);
        }

        if (primitive->indices)
        {
          cgltf_accessor *indexAccessor = primitive->indices;
          size_t indexCount = indexAccessor->count;

          for (size_t i = 0; i < indexCount; i++)
          {
            cgltf_size index = cgltf_accessor_read_index(indexAccessor, i);
            mesh.m_index_data.push_back(baseVertex + static_cast<uint32_t>(index));
          }
        }
        else
        {
          for (size_t i = 0; i < vertexCount; i++)
            mesh.m_index_data.push_back(baseVertex + static_cast<uint32_t>(i));
        }
      }
    }
    cgltf_free(data);
  }
}
