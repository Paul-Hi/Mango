#ifndef MANGO_MATERIAL_GLSL
#define MANGO_MATERIAL_GLSL

#include <bindings.glsl>

layout(binding = MATERIAL_DATA_BUFFER_BINDING_POINT, std140) uniform material_data
{
    vec4  base_color;
    vec4  emissive_color; // this is a vec3, but there are annoying bugs with some drivers.
    float metallic;
    float roughness;
    bool  base_color_texture;
    bool  roughness_metallic_texture;
    bool  occlusion_texture;
    bool  packed_occlusion;
    bool  normal_texture;
    bool  emissive_color_texture;
    float emissive_intensity;
    int   alpha_mode;
    float alpha_cutoff;
};

#endif // MANGO_MATERIAL_GLSL