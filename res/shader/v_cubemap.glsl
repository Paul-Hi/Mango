#version 430 core

layout (location = 0) in vec3 v_position;

layout (location = 0) uniform mat4 u_view_projection_matrix;
layout (location = 1) uniform mat3 u_rotation_scale;

out vec3 shared_texcoord;

void main()
{
    shared_texcoord = v_position;
    vec4 pos = u_view_projection_matrix * vec4(u_rotation_scale * v_position, 1.0);
    gl_Position = pos.xyww;
}
