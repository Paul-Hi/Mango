#include <../include/common_constants_and_functions.glsl>

layout (location = 0) uniform sampler2D sampler_base_color;

// Uniform Buffer Material.
layout(binding = 3, std140) uniform material_data
{
    vec4  base_color;
    vec4  emissive_color; // this is a vec3, but there are annoying bugs with some drivers.
    float metallic;
    float roughness;

    bool base_color_texture;
    bool roughness_metallic_texture;
    bool occlusion_texture;
    bool packed_occlusion;
    bool normal_texture;
    bool emissive_color_texture;

    int   alpha_mode;
    float alpha_cutoff;
};

in shared_data
{
    vec2 texcoord;
} fs_in;

void main()
{
    vec4 color = base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;
    if(alpha_mode == 2 && color.a < 1.0 - 1e-5)
        discard;
    if(alpha_mode == 3)
        alpha_dither(gl_FragCoord.xy, sqrt(color.a));
}