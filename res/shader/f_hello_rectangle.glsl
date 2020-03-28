#version 430 core

out vec4 frag_color;

in vec2 shared_tc;

uniform sampler2D u_color_texture;

void main()
{
    frag_color = texture(u_color_texture, shared_tc);
}
