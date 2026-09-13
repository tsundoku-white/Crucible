#version 450

// ---- Descriptor Set 0 ----
layout(std140, set = 0, binding = 0) uniform CameraUBO
{
  mat4 projection;
  mat4 view;
} ubo;

struct InstanceData
{
  mat4 model;
  uint textureIndex;
};

layout(std430, set = 0, binding = 1) readonly buffer ModelSSBO
{
  InstanceData instances[];
} ssbo;

// ---- Vertex attributes ----
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// ---- Outputs to fragment shader ----
layout(location = 0) out vec3 fragWorldNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) flat out uint fragTexIndex;

void main()
{
  InstanceData inst = ssbo.instances[gl_InstanceIndex];
  mat4 modelMatrix = inst.model;
  fragTexIndex = inst.textureIndex;

  gl_Position = ubo.projection * ubo.view * modelMatrix * vec4(inPosition, 1.0);

  fragWorldPos    = (modelMatrix * vec4(inPosition, 1.0)).xyz;
  fragWorldNormal = mat3(transpose(inverse(modelMatrix))) * inNormal;
  fragUV          = inUV;
}

