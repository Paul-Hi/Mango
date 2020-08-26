#version 430 core

const float PI = 3.1415926535897932384626433832795;
const float INV_PI = 1.0 / PI;

#define saturate(x) clamp(x, 0.0, 1.0)

out vec4 frag_color;

in vec2 texcoord;

layout(location = 0, binding = 0) uniform sampler2D gbuffer_c0;
layout(location = 1, binding = 1) uniform sampler2D gbuffer_c1;
layout(location = 2, binding = 2) uniform sampler2D gbuffer_c2;
layout(location = 3, binding = 3) uniform sampler2D gbuffer_c3;
layout(location = 4, binding = 4) uniform sampler2D gbuffer_depth;

layout(location = 5, binding = 5) uniform samplerCube irradiance_map;
layout(location = 6, binding = 6) uniform samplerCube prefiltered_specular;
layout(location = 7, binding = 7) uniform sampler2D brdf_integration_lut;

layout(binding = 0, std140) uniform lighting_pass_uniforms
{
    mat4 inverse_view_projection;
    vec4 camera_position;  // this is a vec3, but there are annoying bugs with some drivers.
};

layout (location = 8) uniform float ibl_intensity; // TODO Paul: This should probably be put in the lighting uniform buffer as well.

float D_GGX(in float n_dot_h, in float roughness);
float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness);
vec3 F_Schlick(in float dot, in vec3 f0, in float f90);
vec3 F_Schlick_roughness(in float dot, in vec3 f0, in float roughness);
float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness);

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection);
vec3 calculate_image_based_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor);
vec3 calculate_directional_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor);

vec3 get_specular_dominant_direction(in vec3 normal, in vec3 reflection, in float roughness);


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

void main()
{
    float depth = texture(gbuffer_depth, texcoord).r;
    gl_FragDepth = depth; // This is for the potential cubemap.
    if(depth >= 1.0) discard;

    vec3 position                        = world_space_from_depth(depth, texcoord, inverse_view_projection);
    vec4 base_color                      = get_base_color();
    vec3 normal                          = get_normal();
    vec3 occlusion_roughness_metallic    = get_occlusion_roughness_metallic();
    float occlusion_factor               = occlusion_roughness_metallic.r;
    float perceptual_roughness           = occlusion_roughness_metallic.g;
    float metallic                       = occlusion_roughness_metallic.b;
    float reflectance                    = 0.5; // TODO Paul: Make tweakable.

    vec3 f0          = 0.16 * reflectance * reflectance * (1.0 - metallic) + base_color.rgb * metallic;
    vec3 real_albedo = base_color.rgb * (1.0 - metallic);

    vec3 view_dir = normalize(camera_position.xyz - position);
    float n_dot_v = clamp(dot(normal, view_dir), 1e-5, 1.0 - 1e-5);

    vec3 lighting = vec3(0.0);

    // environment
    lighting += calculate_image_based_light(real_albedo, n_dot_v, view_dir, normal, perceptual_roughness, f0, occlusion_factor);

    // lights
    // lighting += calculate_directional_light(real_albedo, n_dot_v, view_dir, normal, perceptual_roughness, f0, occlusion_factor);

    vec3 emissive = get_emissive();
    lighting += emissive * 50000; // TODO Paul: Remove hardcoded intensity for all emissive values -.-

    frag_color = vec4(lighting, base_color.a); // TODO Paul: Proper transparency.
}

vec3 calculate_image_based_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor)
{
    float roughness = sqrt(perceptual_roughness);

    const float DFG_TEXTURE_SIZE = 256.0; // TODO Paul: Hardcoded -.-

    n_dot_v = clamp(n_dot_v , 0.5 / DFG_TEXTURE_SIZE, 1.0 - 0.5 / DFG_TEXTURE_SIZE);
    vec3 dfg = textureLod(brdf_integration_lut, saturate(vec2(n_dot_v, roughness)), 0.0).xyz;
    // irradiance
    vec3 irradiance  = texture(irradiance_map, normal).rgb;
    vec3 diffuse     = irradiance * real_albedo;
    vec3 diffuse_ibl = diffuse;

    // specular
    const float MAX_REFLECTION_LOD = 10.0; // TODO Paul: Hardcoded -.-

    vec3 refl              = -normalize(reflect(view_dir, normal));
    vec3 dominant_refl     = get_specular_dominant_direction(normal, refl, perceptual_roughness);
    float mip_index        = perceptual_roughness * MAX_REFLECTION_LOD;
    vec3 prefiltered_color = textureLod(prefiltered_specular, dominant_refl, mip_index).rgb;
    vec3 specular_ibl      = prefiltered_color * mix(dfg.xxx, dfg.yyy, f0);

    vec3 energy_compensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
    specular_ibl *= energy_compensation;

    return (diffuse_ibl * occlusion_factor + specular_ibl) * ibl_intensity;
}

vec3 calculate_directional_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor)
{
    vec3 light_dir        = vec3(1.0); // hardcoded
    float light_intensity = 111000.0; // hardcoded
    vec3 light_col        = vec3(1.0) * light_intensity; // hardcoded
    float roughness       = (perceptual_roughness * perceptual_roughness);

    vec3 lighting = vec3(0.0);

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

    lighting += (diffuse * occlusion_factor + specular) * light_col;

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
    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

vec3 F_Schlick(in float dot, in vec3 f0, in float f90) // can be optimized
{
    return f0 + (vec3(f90) - f0) * pow(saturate(1.0 - dot), 5.0);
}

vec3 F_Schlick_roughness(in float dot, in vec3 f0, in float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(saturate(1.0 - dot), 5.0);
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

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection)
{
    float z = depth * 2.0 - 1.0;

    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 direct = inverse_view_projection * clip;
    return direct.xyz / direct.w;
}

vec3 get_specular_dominant_direction(in vec3 normal, in vec3 reflection, in float roughness)
{
    float smoothness = saturate(1.0 - roughness);
    float lerp_f = smoothness * (sqrt(smoothness) + roughness);
    return mix(normal, reflection, lerp_f);
}
