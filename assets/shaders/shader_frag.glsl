#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 2) uniform sampler2D texSampler[12];

layout(location = 0) in vec3 fragWorldNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in uint fragTexIndex;

layout(location = 0) out vec4 outColor;

void main()
{
  float textureScale = 1.f;

  vec2 uvX = fragWorldPos.yz * textureScale;
  vec2 uvY = fragWorldPos.xz * textureScale;
  vec2 uvZ = fragWorldPos.xy * textureScale;

  vec4 texX = texture(texSampler[fragTexIndex], uvX);
  vec4 texY = texture(texSampler[fragTexIndex], uvY);
  vec4 texZ = texture(texSampler[fragTexIndex], uvZ);

  vec3 blendWeights = abs(normalize(fragWorldNormal));
  blendWeights = pow(blendWeights, vec3(4.0));
  blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);

  vec4 finalColor = texX * blendWeights.x +
                    texY * blendWeights.y +
                    texZ * blendWeights.z;

  outColor = finalColor;
}
