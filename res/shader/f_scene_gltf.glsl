#version 430 core

layout (location = 0) out vec4 gbuffer_c0; // base_color / reflection_color (rgba8)
layout (location = 1) out vec4 gbuffer_c1; // normal (rgb10)
layout (location = 2) out vec4 gbuffer_c2; // emissive (rgb8) and something else
layout (location = 3) out vec4 gbuffer_c3; // occlusion (r8), roughness (g8), metallic (b8) and something else

in shader_shared
{
    vec3 shared_vertex_position;
    vec2 shared_texcoord;
    vec3 shared_normal;
    vec3 shared_tangent;
    vec3 shared_bitangent;
} fs_in;

layout (location = 1, binding = 0) uniform sampler2D t_base_color;
layout (location = 2, binding = 1) uniform sampler2D t_roughness_metallic;
layout (location = 3, binding = 2) uniform sampler2D t_occlusion;
layout (location = 4, binding = 3) uniform sampler2D t_normal;
layout (location = 5, binding = 4) uniform sampler2D t_emissive_color;

layout(binding = 0, std140) uniform scene_vertex_uniforms
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

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

vec4 get_base_color()
{
    vec4 color = base_color_texture ? texture(t_base_color, fs_in.shared_texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;

    if(alpha_mode == 3)
        alpha_dither(gl_FragCoord.xy, sqrt(color.a));

    return color;
}

vec3 get_emissive()
{
    return emissive_color_texture ? texture(t_emissive_color, fs_in.shared_texcoord).rgb : emissive_color.rgb;
}

vec3 get_occlusion_roughness_metallic()
{
    vec3 o_r_m = roughness_metallic_texture ? texture(t_roughness_metallic, fs_in.shared_texcoord).rgb : vec3(1.0, roughness, metallic);
    if(packed_occlusion)
        return o_r_m;

    float occlusion = occlusion_texture ? texture(t_occlusion, fs_in.shared_texcoord).r : 1.0;
    o_r_m.r = occlusion;

    return o_r_m;
}

vec3 get_normal()
{
    vec3 normal = normalize(fs_in.shared_normal);
    vec3 dfdx = dFdx(fs_in.shared_vertex_position);
    vec3 dfdy = dFdy(fs_in.shared_vertex_position);
    if(!has_normals)
        normal = normalize(cross(dfdx, dfdy)); // approximation
    if(normal_texture)
    {
        vec3 tangent   = fs_in.shared_tangent;
        vec3 bitangent = fs_in.shared_bitangent;
        if(!has_tangents)
        {
            vec3 uv_dx = dFdx(vec3(fs_in.shared_texcoord, 0.0));
            vec3 uv_dy = dFdy(vec3(fs_in.shared_texcoord, 0.0));
            vec3 t_    = (uv_dy.y * dfdx - uv_dx.y * dfdy) / (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);
            tangent    = normalize(t_ - normal * dot(normal, t_));
            bitangent  = cross(normal, tangent);
        }

        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normal);
        vec3 mapped_normal = normalize(texture(t_normal, fs_in.shared_texcoord).rgb * 2.0 - 1.0);
        normal = normalize(tbn * mapped_normal.rgb);
    }
    return normal * 0.5 + 0.5;
}

void main()
{
    gbuffer_c0 = vec4(get_base_color());
    gbuffer_c1 = vec4(get_normal(), 1.0);
    gbuffer_c2 = vec4(get_emissive(), 1.0);
    gbuffer_c3 = vec4(get_occlusion_roughness_metallic(), 1.0);
}
