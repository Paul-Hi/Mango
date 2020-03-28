#version 430 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_texcoord;

uniform mat3 u_rotation_matrix;

out vec2 shared_tc;

void main()
{
    shared_tc = v_texcoord;
    gl_Position = vec4(u_rotation_matrix * v_position, 1.0);
}
