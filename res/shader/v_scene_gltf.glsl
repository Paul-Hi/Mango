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

out shader_shared
{
    vec3 shared_normal;
    vec2 shared_texcoord;
    vec3 shared_tangent;
    vec3 shared_bitangent;
} vs_out;

void main()
{
    vs_out.shared_normal = u_normal_matrix * normalize(v_normal);
    vs_out.shared_texcoord = v_texcoord;
    vs_out.shared_tangent = u_normal_matrix * normalize(v_tangent);
    vs_out.shared_bitangent = cross(vs_out.shared_normal, vs_out.shared_tangent);
    gl_Position = u_view_projection_matrix * u_model_matrix * vec4(v_position, 1.0);
}
