#version 430 core

layout (location = 0) in vec3 v_position;

uniform mat3 u_rotation_matrix;

void main()
{
    gl_Position = vec4(u_rotation_matrix * v_position, 1.0);
}
