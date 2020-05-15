#version 430 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_texcoord;
layout (location = 3) in vec3 v_tangent;

layout(binding = 0, std140) uniform scene_vertex_uniforms
{
    mat4 u_model_matrix;
    mat3 u_normal_matrix;
    mat4 u_view_projection_matrix;
};

out vec3 shared_normal;
out vec2 shared_texcoord;
out vec3 shared_tangent;
out vec3 shared_bitangent;

void main()
{
    shared_normal = u_normal_matrix * normalize(v_normal);
    shared_texcoord = v_texcoord;
    shared_tangent = u_normal_matrix * normalize(v_tangent);
    shared_bitangent = cross(shared_normal, shared_tangent);
    gl_Position = u_view_projection_matrix * u_model_matrix * vec4(v_position, 1.0);
}
