#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec2 frag_uv;
layout(location = 2) in vec3 frag_color;
layout(location = 3) in vec3 frag_world_pos;
layout(location = 4) in vec3 frag_light_pos;
layout(location = 5) flat in int frag_tex_index;

layout(binding = 2) uniform sampler2D textures[];

layout(location = 0) out vec4 outColor;

const vec3 AMBIENT_COLOR = vec3(0.1, 0.1, 0.1);
const vec3 LIGHT_COLOR   = vec3(1.0, 1.0, 1.0);

void main() {
    vec3 normal    = normalize(frag_normal);
    vec3 light_dir = normalize(frag_light_pos - frag_world_pos);

    float diffuse_factor = max(dot(normal, light_dir), 0.0);
    vec3 diffuse  = diffuse_factor * LIGHT_COLOR;
    vec3 ambient  = AMBIENT_COLOR;

    vec3 tex_color = frag_tex_index >= 0
        ? texture(textures[nonuniformEXT(frag_tex_index)], frag_uv).rgb
        : vec3(1.0);

    vec3 lit_color = (ambient + diffuse) * frag_color * tex_color;

    outColor = vec4(lit_color, 1.0);
}
