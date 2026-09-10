#version 450

layout(set = 0, binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormalWorld;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main()
{
  vec3 N = normalize(fragNormalWorld);

  vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
  float diffuse = max(dot(N, lightDir), 0.0);

  vec4 texColor  = texture(texSampler, fragUV);
  vec3 baseColor = texColor.rgb;          // texture replaces the old hardcoded baseColor

  vec3 ambient = 0.15 * baseColor;
  vec3 color   = ambient + diffuse * baseColor;

  outColor = vec4(color, texColor.a);
}
