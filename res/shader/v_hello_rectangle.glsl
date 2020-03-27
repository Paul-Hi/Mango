#version 430 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_color;

uniform mat3 u_rotation_matrix;

out vec3 shared_color;

void main()
{
    shared_color = v_color;
    gl_Position = vec4(u_rotation_matrix * v_position, 1.0);
}
