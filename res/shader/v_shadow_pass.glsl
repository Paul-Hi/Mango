#version 430 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec4 v_tangent;

layout(binding = 0, std140) uniform scene_vertex_uniforms
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};


out shader_shared
{
    vec2 shared_texcoord;
} vs_out;

void main()
{
    vs_out.shared_texcoord = v_texcoord;
    vec4 v_pos = model_matrix * vec4(v_position, 1.0);
    gl_Position = v_pos;
}
