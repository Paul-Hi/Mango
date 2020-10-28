#version 430 core

// Output Textures GBuffer.
layout (location = 0) out vec4 gbuffer_color_target0; // base_color / reflection_color (rgba8)
layout (location = 1) out vec4 gbuffer_color_target1; // normal (rgb10)
layout (location = 2) out vec4 gbuffer_color_target2; // emissive (rgb8) and something else
layout (location = 3) out vec4 gbuffer_color_target3; // occlusion (r8), roughness (g8), metallic (b8) and something else

// Shared Shader Data.
in shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
} fs_in;

// Texture Samplers.
layout (location = 0, binding = 0) uniform sampler2D sampler_base_color;
layout (location = 1, binding = 1) uniform sampler2D sampler_roughness_metallic;
layout (location = 2, binding = 2) uniform sampler2D sampler_occlusion;
layout (location = 3, binding = 3) uniform sampler2D sampler_normal;
layout (location = 4, binding = 4) uniform sampler2D sampler_emissive_color;

// Uniform Buffer Renderer.
layout(binding = 0, std140) uniform renderer_data
{
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    float camera_exposure;
};

// Uniform Buffer Model.
layout(binding = 1, std140) uniform model_data
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

// Uniform Buffer Material.
layout(binding = 2, std140) uniform material_data
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
    vec4 color = base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;

    if(alpha_mode == 3)
        alpha_dither(gl_FragCoord.xy, sqrt(color.a));

    return color;
}

vec3 get_emissive()
{
    return emissive_color_texture ? texture(sampler_emissive_color, fs_in.texcoord).rgb : emissive_color.rgb;
}

vec3 get_occlusion_roughness_metallic()
{
    vec3 o_r_m = roughness_metallic_texture ? texture(sampler_roughness_metallic, fs_in.texcoord).rgb : vec3(1.0, roughness, metallic);
    if(packed_occlusion)
        return o_r_m;

    float occlusion = occlusion_texture ? texture(sampler_occlusion, fs_in.texcoord).r : 1.0;
    o_r_m.r = occlusion;

    return o_r_m;
}

vec3 get_normal()
{
    vec3 normal = normalize(fs_in.normal);
    vec3 dfdx = dFdx(fs_in.position);
    vec3 dfdy = dFdy(fs_in.position);
    if(!has_normals)
        normal = normalize(cross(dfdx, dfdy)); // approximation
    if(normal_texture)
    {
        vec3 tangent   = fs_in.tangent;
        vec3 bitangent = fs_in.bitangent;
        if(!has_tangents)
        {
            vec3 uv_dx = dFdx(vec3(fs_in.texcoord, 0.0));
            vec3 uv_dy = dFdy(vec3(fs_in.texcoord, 0.0));
            vec3 t_    = (uv_dy.y * dfdx - uv_dx.y * dfdy) / (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);
            tangent    = normalize(t_ - normal * dot(normal, t_));
            bitangent  = cross(normal, tangent);
        }

        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normal);
        vec3 mapped_normal = normalize(texture(sampler_normal, fs_in.texcoord).rgb * 2.0 - 1.0);
        normal = normalize(tbn * mapped_normal.rgb);
    }
    if(!gl_FrontFacing)
        normal *= -1.0;
    return normal * 0.5 + 0.5;
}

void main()
{
    gbuffer_color_target0 = vec4(get_base_color());
    gbuffer_color_target1 = vec4(get_normal(), 1.0);
    gbuffer_color_target2 = vec4(get_emissive(), 1.0);
    gbuffer_color_target3 = vec4(get_occlusion_roughness_metallic(), 1.0);
}
