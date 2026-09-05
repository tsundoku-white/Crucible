#version 450

// ---- Descriptor Set 0 ----
// Binding 0: camera UBO — matches n_resource::UniformBufferObject exactly
// (mat4 projection, mat4 view, std140 layout, both 16-byte aligned already).
layout(std140, set = 0, binding = 0) uniform CameraUBO
{
  mat4 projection;
  mat4 view;
} ubo;

// Binding 1: per-draw model matrix SSBO — matches the FRONT of
// n_resource::ShaderStorageBufferObject (mat4 modelsMatrix). We only read
// the leading mat4; the trailing bookkeeping fields (modelCount, indices,
// indicesCount) aren't needed here and are simply unused tail bytes.
layout(std430, set = 0, binding = 1) readonly buffer ModelSSBO
{
  mat4 model;
} ssbo;

// ---- Vertex attributes ----
// Matches Vertex::get_attributeDescriptions() in buffer.h exactly:
//   location 0 = m_pos    (vec3)
//   location 1 = m_normal (vec3)
//   location 2 = m_uv     (vec2)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// ---- Outputs to fragment shader ----
layout(location = 0) out vec3 fragNormalWorld;
layout(location = 1) out vec2 fragUV;

void main()
{
  mat4 modelMatrix = ssbo.model;

  vec4 worldPos = modelMatrix * vec4(inPosition, 1.0);
  gl_Position = ubo.projection * ubo.view * worldPos;

  // Normal matrix (inverse-transpose) to correctly handle non-uniform scale.
  mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
  fragNormalWorld = normalize(normalMatrix * inNormal);

  fragUV = inUV;
}
