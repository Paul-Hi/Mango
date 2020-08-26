#version 430 core

out vec4 frag_color;

in vec3 shared_texcoord;

layout (location = 2, binding = 0) uniform samplerCube skybox;
layout (location = 3) uniform float intensity;
layout (location = 4) uniform float mip_level;

void main()
{
    vec3 color = (textureLod(skybox, shared_texcoord, mip_level) * intensity).rgb;

    frag_color = vec4(color, 1.0);
}
