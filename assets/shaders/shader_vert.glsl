#version 450

layout(location = 0) in vec3 in_loc;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec3 in_color;

layout(binding = 0) uniform UBO {
    mat4 projection;
    mat4 view;
    vec3 light_pos; 
} ubo;

layout(binding = 1) readonly buffer SSUBO {
    mat4 model[];
} ssubo;

layout(binding = 3) readonly buffer TexIndexSSBO {
    int tex_index[];
} tex_ssbo;

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec2 frag_uv;
layout(location = 2) out vec3 frag_color;
layout(location = 3) out vec3 frag_world_pos;
layout(location = 4) out vec3 frag_light_pos;
layout(location = 5) flat out int frag_tex_index;

void main() {
    mat4 model = ssubo.model[gl_InstanceIndex];
    vec4 world_pos = model * vec4(in_loc, 1.0);

    gl_Position = ubo.projection * ubo.view * world_pos;

    mat3 normal_matrix = mat3(transpose(inverse(model)));

    float det = determinant(mat3(model));
    if (det < 0.0) {
        normal_matrix = -normal_matrix;
        gl_Position.w = -gl_Position.w;
    }

    frag_normal = normalize(normal_matrix * in_normal);
    frag_uv = in_uv;
    frag_color = in_color;
    frag_world_pos = world_pos.xyz;
    frag_light_pos = ubo.light_pos;
    frag_tex_index = tex_ssbo.tex_index[gl_InstanceIndex];
}
