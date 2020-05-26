#version 430 core

out vec4 frag_color;

in vec3 shared_texcoord;

layout (location = 1, binding = 0) uniform samplerCube skybox;
layout (location = 2) uniform float mip_level;

void main()
{
    vec3 color = textureLod(skybox, shared_texcoord, mip_level).rgb;
    // tonemapping // TODO Paul: There is room for improvement. For exapmle a white point parameter.
    // modified reinhard
    const float white_point = 1.0;
    color = (color * (1.0 + color / (white_point * white_point))) / (1.0 + color);
    color = pow(color, vec3(1.0 / 2.2));
    frag_color = vec4(color, 1.0);
}
