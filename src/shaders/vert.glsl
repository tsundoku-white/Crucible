#version 450

layout(location = 0) in vec3 in_loc;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec3 in_color;

layout(binding = 0) uniform ShaderData {
    mat4 projection;
    mat4 view;
    mat4 model[3];
    vec4 light;
} ubo;

layout(push_constant) uniform PushConsts {
    int instance_index; 
} pc;

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec2 frag_uv;
layout(location = 2) out vec3 frag_color;
layout(location = 3) out vec3 frag_world_pos;

void main() {
    mat4 model = ubo.model[pc.instance_index];
    vec4 world_pos = model * vec4(in_loc, 1.0);

    gl_Position = ubo.projection * ubo.view * world_pos;

    frag_normal    = mat3(transpose(inverse(model))) * in_normal;
    frag_uv        = in_uv;
    frag_color     = in_color;
    frag_world_pos = world_pos.xyz;
}
