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

void alpha_dither(in vec2 screen_pos, in float alpha) {

    const float thresholds[16] =
    {
        1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
    };

    int x = int(mod(screen_pos.x, 4.0));
    int y = int(mod(screen_pos.y, 4.0));

    float limit = thresholds[x + (y * 4)];

    if(alpha < limit)
        discard;
}

void main()
{
    vec4 color = base_color_texture ? texture(t_base_color, fs_in.shared_texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;
    if(alpha_mode == 3)
        alpha_dither(gl_FragCoord.xy, sqrt(color.a));
}