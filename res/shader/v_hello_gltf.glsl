#version 430 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;

uniform mat4 u_model_matrix;
uniform mat4 u_view_projection_matrix;

out vec3 shared_normal;

void main()
{
    shared_normal = normalize(v_normal);
    gl_Position = u_view_projection_matrix * u_model_matrix * vec4(v_position, 1.0);
}
