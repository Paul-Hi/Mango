#version 430 core

out vec4 frag_color;

in shader_shared
{
    vec3 position;
} fs_in;

void main()
{
    frag_color = vec4(fs_in.position, 1.0f);
}
