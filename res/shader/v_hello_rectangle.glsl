#version 430 core

layout (location = 0) in vec3 v_position;

out shader_shared
{
    vec3 position;
} vs_out;

void main()
{
    vs_out.position = v_position;
    gl_Position = vec4(v_position, 1.0);
}
