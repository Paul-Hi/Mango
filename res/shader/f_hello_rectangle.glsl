#version 430 core

out vec4 frag_color;

uniform vec3 u_color = vec3(1.0f);

void main()
{
    frag_color = vec4(u_color, 1.0f);
}
