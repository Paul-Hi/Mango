#version 430 core

#define PI 3.14159265358979323846

#define saturate(x) clamp(x, 0.0, 1.0)

out vec4 frag_color;

in vec2 texcoord;

layout (location = 0) uniform mat4 u_inverse_view_projection;
layout (location = 1) uniform vec3 u_camera_position;

layout(location = 2, binding = 0) uniform sampler2D gbuffer_c0;
layout(location = 3, binding = 1) uniform sampler2D gbuffer_c1;
layout(location = 4, binding = 2) uniform sampler2D gbuffer_c2;
layout(location = 5, binding = 3) uniform sampler2D gbuffer_c3;
layout(location = 6, binding = 4) uniform sampler2D gbuffer_depth;

float D_GGX(in float n_dot_h, in float roughness);
float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness);
vec3 F_Schlick(in float dot, in vec3 f0, in float f90);
float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness);

vec3 calculateTestLight(in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in vec3 real_albedo, in vec3 position, in float occlusion_factor);

vec4 get_base_color()
{
    return texture(gbuffer_c0, texcoord);
}

vec3 get_normal()
{
    return normalize(texture(gbuffer_c1, texcoord).rgb * 2.0 - 1.0);
}

vec3 get_emissive()
{
    return texture(gbuffer_c2, texcoord).rgb;
}

vec3 get_occlusion_roughness_metallic()
{
    vec3 o_r_m = texture(gbuffer_c3, texcoord).rgb;
    o_r_m.x = max(o_r_m.x, 0.089f);
    return o_r_m;
}

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection)
{
    float z = depth * 2.0 - 1.0;

    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 direct = inverse_view_projection * clip;
    return direct.xyz / direct.w;
}

void main()
{
    float depth = texture(gbuffer_depth, texcoord).r;
    if(depth >= 1.0) discard;
    vec3 emissive = get_emissive();
    if(length(emissive) > 0.0)
    {
        frag_color = vec4(saturate(pow(emissive, vec3(1.0 / 2.2))), 1.0);
        return;
    }

    vec3 position                        = world_space_from_depth(depth, texcoord, u_inverse_view_projection);
    vec3 albedo                          = get_base_color().rgb;
    vec3 normal                          = get_normal();
    vec3 occlusion_roughness_metallic    = get_occlusion_roughness_metallic();
    float occlusion_factor               = occlusion_roughness_metallic.r;
    float perceptual_roughness           = occlusion_roughness_metallic.g;
    float metallic                       = occlusion_roughness_metallic.b;
    vec3 reflectance                     = albedo;

    vec3 f0          = 0.16 * reflectance * reflectance * (1.0 - metallic) + albedo * metallic;
    vec3 real_albedo = albedo * (1.0 - metallic);

    vec3 view_dir = normalize(u_camera_position - position);
    float n_dot_v = clamp(dot(normal, view_dir), 1e-5, 1.0 - 1e-5);

    vec3 lighting = vec3(0.0);

    vec3 ambient = 0.02 * real_albedo;
    lighting += ambient * occlusion_factor;

    lighting += calculateTestLight(n_dot_v, view_dir, normal, perceptual_roughness, f0, real_albedo, position, occlusion_factor);

    // improvising gamma correction
    lighting = pow(lighting, vec3(1.0 / 2.2));

    frag_color = vec4(saturate(lighting), 1.0);
}


vec3 calculateTestLight(in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in vec3 real_albedo, in vec3 position, in float occlusion_factor)
{
    vec3 light_pos[4] = { vec3(-55.0, 35.0,  45.0),
                          vec3( 55.0, 35.0,  45.0),
                          vec3(-55.0, 35.0, -45.0),
                          vec3( 55.0, 35.0, -45.0)
    }; // hardcoded
    float light_intensity      = 1.0; // hardcoded
    vec3 light_col             = vec3(1.0) * light_intensity; // hardcoded
    float roughness            = perceptual_roughness * perceptual_roughness;

    vec3 lighting = vec3(0.0);

    for(uint i = 0; i < 4; ++i)
    {
        vec3 light_dir = normalize(light_pos[i] - position);

        vec3 halfway  = normalize(light_dir + view_dir);
        float n_dot_l = saturate(dot(normal, light_dir));
        float n_dot_h = saturate(dot(normal, halfway));
        float l_dot_h = saturate(dot(light_dir, halfway));

        float D = D_GGX(n_dot_h, roughness);
        vec3 F  = F_Schlick(l_dot_h, f0, 1.0);
        float V = V_SmithGGXCorrelated(n_dot_v, n_dot_l, roughness);

        // Fr energy compensation
        vec3 Fr = D * V * F * (1.0 / PI);

        vec3 Fd = real_albedo * Fd_BurleyRenormalized(n_dot_v, n_dot_l, l_dot_h, perceptual_roughness) * (1.0 / PI);

        vec3 diffuse = n_dot_l * Fd;
        vec3 specular = n_dot_l * Fr;

        lighting += (diffuse + specular) * light_col * occlusion_factor;
    }

    return lighting;
}

//
// see https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf.
//

float D_GGX(in float n_dot_h, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float f = (n_dot_h * a_sqr - n_dot_h) * n_dot_h + 1.0;
    // Gets divided by pi later on.
    return a_sqr / (f * f);
}

float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float GGXL = n_dot_v * sqrt((-n_dot_l * a_sqr + n_dot_l) * n_dot_l + a_sqr);
    float GGXV = n_dot_l * sqrt((-n_dot_v * a_sqr + n_dot_v) * n_dot_v + a_sqr);
    return 0.5 / (GGXV + GGXL);
}

vec3 F_Schlick(in float dot, in vec3 f0, in float f90) // can be optimized
{
    return f0 + (vec3(f90) - f0) * pow(1.0 - dot, 5.0);
}

float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness) // normalized Frostbyte version
{
    float energy_bias = mix(0.0, 0.5, roughness);
    float energy_factor = mix(1.0, 1.0 / 1.51, roughness);
    float f90 = energy_bias + 2.0 * l_dot_h * l_dot_h * roughness;
    vec3 f0 = vec3(1.0);
    float light_scatter = F_Schlick(n_dot_l, f0, f90).x;
    float view_scatter = F_Schlick(n_dot_v, f0, f90).x;
    // Gets divided by pi later on.
    return energy_factor * light_scatter * view_scatter;
}
