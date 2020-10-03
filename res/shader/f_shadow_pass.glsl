#version 430 core


layout (location = 5, binding = 0) uniform sampler2D t_base_color;

layout(binding = 1, std140) uniform scene_material_uniforms
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

    int alpha_mode;
    float alpha_cutoff;
};

in shader_shared
{
    vec2 shared_texcoord;
} fs_in;


void main()
{
    vec4 color = base_color_texture ? texture(t_base_color, fs_in.shared_texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;
}