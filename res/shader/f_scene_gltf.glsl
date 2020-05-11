#version 430 core

layout (location = 0) out vec4 gbuffer_c0; // base_color
layout (location = 1) out vec4 gbuffer_c1; // normal, roughness
layout (location = 2) out vec4 gbuffer_c2; // metallic

in vec3 shared_normal;
in vec2 shared_texcoord;

layout (location = 0, binding = 0) uniform sampler2D t_base_color;
layout (location = 1, binding = 1) uniform sampler2D t_metallic;
layout (location = 2, binding = 2) uniform sampler2D t_roughness;
layout (location = 3, binding = 3) uniform sampler2D t_normal;

layout(binding = 1, std140) uniform scene_material_uniforms
{
    vec4  base_color;
    float metallic;
    float roughness;

    bool base_color_texture;
    bool metallic_texture;
    bool roughness_texture;
    bool normal_texture;
};

vec4 get_base_color()
{
    return base_color_texture ? texture(t_base_color, shared_texcoord) : base_color;
}

float get_metallic()
{
    return metallic_texture ? texture(t_metallic, shared_texcoord).r : metallic;
}

float get_roughness()
{
    return roughness_texture ? texture(t_roughness, shared_texcoord).r : roughness;
}

vec3 get_normal()
{
    return normal_texture ? texture(t_normal, shared_texcoord).rgb : shared_normal;
}

void main()
{
    gbuffer_c0 = get_base_color();
    gbuffer_c1 = vec4(normalize(get_normal()) * 0.5f + 0.5f, get_roughness());
    gbuffer_c2 = vec4(get_metallic(), 0.0f, 0.0f, 0.0f);
}
