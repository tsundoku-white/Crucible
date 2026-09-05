#version 450

layout(location = 0) in vec3 fragNormalWorld;
layout(location = 1) in vec2 fragUV; // unused for now (no texture bound yet)

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 N = normalize(fragNormalWorld);

  // Simple fixed directional light, just to make geometry visually readable
  // until real lighting/materials are wired up.
  vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
  float diffuse = max(dot(N, lightDir), 0.0);

  vec3 baseColor = vec3(0.8, 0.8, 0.85);
  vec3 ambient   = 0.15 * baseColor;
  vec3 color     = ambient + diffuse * baseColor;

  outColor = vec4(color, 1.0);
}
