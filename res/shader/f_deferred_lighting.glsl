#version 430 core

const float PI = 3.1415926535897932384626433832795;
const float INV_PI = 1.0 / PI;

#define saturate(x) clamp(x, 0.0, 1.0)
#define max_cascades 4

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

layout(location = 8, binding = 8) uniform sampler2DArray shadow_map;

// Uniform Buffer Lighting Pass.
layout(binding = 3, std140) uniform lighting_pass_data
{
    mat4 inverse_view_projection;
    mat4 view;
    vec4 camera_position; // this is a vec3, but there are annoying bugs with some drivers.
    vec4 camera_params; // near, far, (zw) unused

    vec4  directional_direction; // this is a vec3, but there are annoying bugs with some drivers.
    vec4  directional_color; // this is a vec3, but there are annoying bugs with some drivers.
    float directional_intensity;
    bool  cast_shadows;

    float ambient_intensity;

    bool debug_view_enabled;
    bool debug_views_position;
    bool debug_views_normal;
    bool debug_views_depth;
    bool debug_views_base_color;
    bool debug_views_reflection_color;
    bool debug_views_emission;
    bool debug_views_occlusion;
    bool debug_views_roughness;
    bool debug_views_metallic;
    bool show_cascades;
    bool draw_shadow_maps;
};

// Uniform Buffer Shadow.
layout(binding = 4, std140) uniform shadow_data
{
    mat4  view_projection_matrices[max_cascades];
    float split_depth[max_cascades + 1];
    vec4  far_planes;
    int   resolution;
    int   cascade_count;
    float shadow_cascade_interpolation_range;
    int sample_count;
    float slope_bias;
    float normal_bias;
    int filter_mode;
} shadow_map_data;

float D_GGX(in float n_dot_h, in float roughness);
float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness);
vec3 F_Schlick(in float dot, in vec3 f0, in float f90);
vec3 F_Schlick_roughness(in float dot, in vec3 f0, in float roughness);
float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness);

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection);
vec3 calculate_image_based_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor);
vec3 calculate_directional_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor, in vec3 world_pos);
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

void debug_views(in float depth);

// interpolation_mode: 0 -> no interpolation | 1 -> interpolation from cascade_id to cascade_id + 1 - shadow_map_data.shadow_cascade_interpolation_range | 2 -> interpolation from cascade_id - 1  + shadow_map_data.shadow_cascade_interpolation_range to cascade_id
int compute_cascade_id(in float view_depth, out float interpolation_factor, out int interpolation_mode) // xy texcoords, z depth
{
    interpolation_mode = 0;
    int cascade_id = 0;
    int num_cascades = shadow_map_data.cascade_count;
    for(int i = 1; i <= num_cascades; ++i)
    {
        float i_value = shadow_map_data.split_depth[i];
        if(view_depth <= i_value)
        {
            cascade_id = i - 1;
            float i_minus_one_value = shadow_map_data.split_depth[i - 1];
            float mid = (i_value + i_minus_one_value) * 0.5;
            if(view_depth > mid && view_depth > i_value - shadow_map_data.shadow_cascade_interpolation_range)
            {
                interpolation_mode = 1;
                interpolation_factor = (i_value - view_depth) / shadow_map_data.shadow_cascade_interpolation_range;
            }
            if(view_depth < mid && view_depth < i_minus_one_value + shadow_map_data.shadow_cascade_interpolation_range)
            {
                interpolation_mode = 2;
                interpolation_factor = (view_depth - i_minus_one_value) / shadow_map_data.shadow_cascade_interpolation_range;
            }
            break;
        }
    }

    interpolation_factor = (1.0 - interpolation_factor);

    if(cascade_id == 0 && interpolation_mode == 2)
        interpolation_mode = 0;
    if(cascade_id == num_cascades - 1 && interpolation_mode == 1)
        interpolation_mode = 0;

    return cascade_id;
}

float linearize_depth(in float d, in float near, in float far)
{
    float depth = 2.0 * d - 1.0;
    return 2.0 * near * far / (far + near - depth * (far - near));
}

// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
const vec2 poisson_disk[64] = {
    vec2( -0.04117257, -0.1597612 ),
    vec2( 0.06731031,  -0.4353096 ),
    vec2( -0.206701,   -0.4089882 ),
    vec2( 0.1857469,   -0.2327659 ),
    vec2( -0.2757695,   -0.159873 ),
    vec2( -0.2301117,   0.1232693 ),
    vec2( 0.05028719,   0.1034883 ),
    vec2( 0.236303,    0.03379251 ),
    vec2( 0.1467563,     0.364028 ),
    vec2( 0.516759,     0.2052845 ),
    vec2( 0.2962668,    0.2430771 ),
    vec2( 0.3650614,   -0.1689287 ),
    vec2( 0.5764466,  -0.07092822 ),
    vec2( -0.5563748,  -0.4662297 ),
    vec2( -0.3765517,  -0.5552908 ),
    vec2( -0.4642121,   -0.157941 ),
    vec2( -0.2322291,  -0.7013807 ),
    vec2( -0.05415121, -0.6379291 ),
    vec2( -0.7140947,  -0.6341782 ),
    vec2( -0.4819134,  -0.7250231 ),
    vec2( -0.7627537,  -0.3445934 ),
    vec2( -0.7032605,    -0.13733 ),
    vec2( 0.8593938,    0.3171682 ),
    vec2( 0.5223953,    0.5575764 ),
    vec2( 0.7710021,    0.1543127 ),
    vec2( 0.6919019,    0.4536686 ),
    vec2( 0.3192437,    0.4512939 ),
    vec2( 0.1861187,     0.595188 ),
    vec2( 0.6516209,   -0.3997115 ),
    vec2( 0.8065675,   -0.1330092 ),
    vec2( 0.3163648,    0.7357415 ),
    vec2( 0.5485036,    0.8288581 ),
    vec2( -0.2023022,  -0.9551743 ),
    vec2( 0.165668,    -0.6428169 ),
    vec2( 0.2866438,   -0.5012833 ),
    vec2( -0.5582264,   0.2904861 ),
    vec2( -0.2522391,    0.401359 ),
    vec2( -0.428396,    0.1072979 ),
    vec2( -0.06261792,  0.3012581 ),
    vec2( 0.08908027,  -0.8632499 ),
    vec2( 0.9636437,   0.05915006 ),
    vec2( 0.8639213,    -0.309005 ),
    vec2( -0.03422072,  0.6843638 ),
    vec2( -0.3734946,  -0.8823979 ),
    vec2( -0.3939881,   0.6955767 ),
    vec2( -0.4499089,   0.4563405 ),
    vec2( 0.07500362,   0.9114207 ),
    vec2( -0.9658601,  -0.1423837 ),
    vec2( -0.7199838,   0.4981934 ),
    vec2( -0.8982374,   0.2422346 ),
    vec2( -0.8048639,  0.01885651 ),
    vec2( -0.8975322,   0.4377489 ),
    vec2( -0.7135055,   0.1895568 ),
    vec2( 0.4507209,   -0.3764598 ),
    vec2( -0.395958,   -0.3309633 ),
    vec2( -0.6084799,  0.02532744 ),
    vec2( -0.2037191,   0.5817568 ),
    vec2( 0.4493394,   -0.6441184 ),
    vec2( 0.3147424,   -0.7852007 ),
    vec2( -0.5738106,   0.6372389 ),
    vec2( 0.5161195,   -0.8321754 ),
    vec2( 0.6553722,   -0.6201068 ),
    vec2( -0.2554315,   0.8326268 ),
    vec2( -0.5080366,   0.8539945 )
};

#define LIGHT_SIZE 0.035

vec2 sample_blocker(in vec3 shadow_coords, in int cascade_id, in sampler2DArray lookup, in int sample_count)
{
    float avg_blocker_depth = 0.0;
    int blocker_count = 0;
    float search_width = LIGHT_SIZE;
    for (int i = 0; i < sample_count; ++i)
    {
        vec2 sample_uv = shadow_coords.xy + poisson_disk[i] * search_width;
        bvec2 outside = greaterThan(sample_uv, vec2(1.0));
        bvec2 inside = lessThan(sample_uv, vec2(0.0));
        if(any(outside) || any(inside))
            continue;
        float z = texture(lookup, vec3(sample_uv, cascade_id)).x;
        if (z < shadow_coords.z)
        {
            avg_blocker_depth += z;
            blocker_count++;
        }
    }
    return vec2(avg_blocker_depth / blocker_count, blocker_count);
}

float calculate_penumbra_size(in float receiver_z, in float blocker_z, in int cascade_id)
{
    float rd = linearize_depth(receiver_z, 1e-5, shadow_map_data.far_planes[cascade_id]);
    float bd = linearize_depth(blocker_z, 1e-5, shadow_map_data.far_planes[cascade_id]);
    return (rd - bd) / bd;
}

float interleaved_gradient_noise(in vec2 screen_pos)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screen_pos, magic.xy)));
}

float pcf(in vec3 shadow_coords, in int cascade_id, in sampler2DArray lookup, in int sample_count, in float filter_radius)
{
    float sum = 0.0;
    float theta = interleaved_gradient_noise(gl_FragCoord.xy);
    mat2 rotation = mat2(vec2(cos(theta), sin(theta)), vec2(-sin(theta), cos(theta)));
    for(int i = 0; i < sample_count; ++i)
    {
        vec2 sample_uv = shadow_coords.xy + (rotation * poisson_disk[i]) * filter_radius;
        bvec2 outside = greaterThan(sample_uv, vec2(1.0));
        bvec2 inside = lessThan(sample_uv, vec2(0.0));
        if(any(outside) || any(inside))
        {
            sum += 1.0;
            continue;
        }
        float z = texture(lookup, vec3(sample_uv, cascade_id)).x;
        sum += (z < shadow_coords.z) ? 0.0 : 1.0;
    }

    return sum / sample_count;
}


float pcss(in vec3 shadow_coords, in int cascade_id, in sampler2DArray lookup, in int sample_count)
{
    int num_blocker_samples = int(max(sample_count * 0.25, 16.0));
    vec2 blocker_data = sample_blocker(shadow_coords, cascade_id, lookup, num_blocker_samples);
    if(blocker_data.y < 1)
        return 1.0;
    float penumbra_ratio = calculate_penumbra_size(shadow_coords.z, blocker_data.x, cascade_id);
    float filter_radius = penumbra_ratio * LIGHT_SIZE;
    filter_radius = clamp(filter_radius, 1.0 / shadow_map_data.resolution, 6.0 / 2048.0);
    return pcf(shadow_coords, cascade_id, lookup, sample_count, filter_radius);
}

float calculate_shadow(in vec3 shadow_coords, in int cascade_id, in sampler2DArray lookup)
{
    int num_samples = max(shadow_map_data.sample_count, 16);
    switch(shadow_map_data.filter_mode)
    {
        case 1:
            return pcf(shadow_coords, cascade_id, lookup, num_samples, 2.0 / min(shadow_map_data.resolution, 2048));
        break;
        case 2:
            return pcf(shadow_coords, cascade_id, lookup, num_samples, 4.0 / min(shadow_map_data.resolution, 2048));
        break;
        case 3:
            return pcss(shadow_coords, cascade_id, lookup, num_samples);
        break;
        case 0:
        default:
            float z = texture(lookup, vec3(shadow_coords.xy, cascade_id)).x;
            return (z < shadow_coords.z) ? 0.0 : 1.0;
    }
}

const mat4 bias_matrix = mat4(
    vec4(0.5,0,0,0),
    vec4(0,0.5,0,0),
    vec4(0,0,0.5,0),
    vec4(0.5,0.5,0.5,1.0));

float directional_shadow(in sampler2DArray lookup, in vec3 world_pos, in int cascade_id, in float interpolation_factor, in int interpolation_mode, in vec3 bias)
{
    world_pos += bias;
    float shadow = 1.0;


    if(interpolation_mode == 0)
    {
        vec4 projected = bias_matrix * shadow_map_data.view_projection_matrices[cascade_id] * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;

        if(shadow_coords.z > 1.0)
            return shadow;

        shadow = calculate_shadow(shadow_coords, cascade_id, lookup);
    }
    else if(interpolation_mode == 1)
    {
        vec4 projected = bias_matrix * shadow_map_data.view_projection_matrices[cascade_id] * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = calculate_shadow(shadow_coords, cascade_id, lookup);

        vec4 projected2 = bias_matrix * shadow_map_data.view_projection_matrices[cascade_id + 1] * vec4(world_pos, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;

        if(shadow_coords2.z > 1.0)
            return calculate_shadow(shadow_coords, cascade_id, lookup);

        float pcss_shadows2 = calculate_shadow(shadow_coords2, cascade_id + 1, lookup);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }
    else if(interpolation_mode == 2)
    {
        vec4 projected = bias_matrix * shadow_map_data.view_projection_matrices[cascade_id] * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = calculate_shadow(shadow_coords, cascade_id, lookup);

        vec4 projected2 = bias_matrix * shadow_map_data.view_projection_matrices[cascade_id - 1] * vec4(world_pos, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;

        if(shadow_coords2.z > 1.0)
            return calculate_shadow(shadow_coords, cascade_id, lookup);

        float pcss_shadows2 = calculate_shadow(shadow_coords2, cascade_id - 1, lookup);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }

    return shadow;
}

void main()
{
    float depth = texture(gbuffer_depth, texcoord).r;
    if(debug_view_enabled)
    {
        debug_views(depth);
        return;
    }
    bool debug_shadow_maps_viewport = draw_shadow_maps && cast_shadows && texcoord.y < 0.25;
    if(debug_shadow_maps_viewport)
    {
        bool sm3 = texcoord.x > 0.75;
        bool sm2 = texcoord.x > 0.5;
        bool sm1 = texcoord.x > 0.25;
        if(sm3)
        {
            vec2 uv = vec2((texcoord.x - 0.75) / 0.25, texcoord.y / 0.25);
            frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 3)).x), 1.0);
            return;
        }
        if(sm2)
        {
            vec2 uv = vec2((texcoord.x - 0.5) / 0.25, texcoord.y / 0.25);
            frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 2)).x), 1.0);
            return;
        }
        if(sm1)
        {
            vec2 uv = vec2((texcoord.x - 0.25) / 0.25, texcoord.y / 0.25);
            frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 1)).x), 1.0);
            return;
        }
        vec2 uv = vec2((texcoord.x) / 0.25, texcoord.y / 0.25);
        frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 0)).x), 1.0);
        return;
    }

    gl_FragDepth = depth; // This is for potential transparent objects and cubemap.
    if(depth >= 1.0) discard;

    vec3 position                        = world_space_from_depth(depth, texcoord, inverse_view_projection);
    vec4 base_color                      = get_base_color();
    vec3 normal                          = get_normal();
    vec3 occlusion_roughness_metallic    = get_occlusion_roughness_metallic();
    float occlusion_factor               = occlusion_roughness_metallic.r;
    float perceptual_roughness           = occlusion_roughness_metallic.g;
    float metallic                       = occlusion_roughness_metallic.b;
    vec3 emissive                        = get_emissive();
    float reflectance                    = 0.5; // TODO Paul: Make tweakable.

    vec3 f0          = 0.16 * reflectance * reflectance * (1.0 - metallic) + base_color.rgb * metallic;
    vec3 real_albedo = base_color.rgb * (1.0 - metallic);

    vec3 view_dir = normalize(camera_position.xyz - position);
    float n_dot_v = clamp(dot(normal, view_dir), 1e-5, 1.0 - 1e-5);

    vec3 lighting = vec3(0.0);

    // environment
    lighting += calculate_image_based_light(real_albedo, n_dot_v, view_dir, normal, perceptual_roughness, f0, occlusion_factor);

    // lights
    lighting += calculate_directional_light(real_albedo, n_dot_v, view_dir, normal, perceptual_roughness, f0, occlusion_factor, position);

    lighting += emissive * 50000.0; // TODO Paul: Remove hardcoded intensity for all emissive values -.-

    frag_color = vec4(lighting, 1.0);
}

vec3 calculate_image_based_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor)
{
    const float DFG_TEXTURE_SIZE = 256.0; // TODO Paul: Hardcoded -.-

    n_dot_v = clamp(n_dot_v , 0.5 / DFG_TEXTURE_SIZE, 1.0 - 0.5 / DFG_TEXTURE_SIZE);
    vec3 dfg = textureLod(brdf_integration_lut, saturate(vec2(n_dot_v, perceptual_roughness)), 0.0).xyz;
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

    return (diffuse_ibl * occlusion_factor + specular_ibl) * ambient_intensity;
}

vec3 calculate_directional_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor, in vec3 world_pos)
{
    float light_intensity = directional_intensity;
    if(light_intensity < 1e-5)
        return vec3(0.0);
    vec3 light_dir        = normalize(directional_direction.xyz);
    vec3 light_col        = directional_color.rgb;
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

    vec3 Fd = real_albedo * Fd_BurleyRenormalized(n_dot_v, n_dot_l, l_dot_h, roughness) * (1.0 / PI);

    vec3 diffuse = n_dot_l * Fd;
    vec3 specular = n_dot_l * Fr;

    lighting += (diffuse * occlusion_factor + specular) * light_col * light_intensity;

    if(cast_shadows)
    {
        vec4 view_pos = view * vec4(world_pos, 1.0);
        int interpolation_mode;
        float interpolation_factor;
        int cascade_id = compute_cascade_id(abs(view_pos.z), interpolation_factor, interpolation_mode);
        vec3 bias = vec3(clamp(shadow_map_data.slope_bias * tan(acos(n_dot_l)), 0.0,shadow_map_data.slope_bias * 2.0)) + get_normal() * shadow_map_data.normal_bias;

        float shadow = cast_shadows ? directional_shadow(shadow_map, world_pos, cascade_id, interpolation_factor, interpolation_mode, bias) : 1.0;
        lighting = lighting * shadow;
        if(show_cascades)
        {
            if(cascade_id == 0) lighting.gb *= vec2(0.25);
            if(cascade_id == 1) lighting.rb *= vec2(0.25);
            if(cascade_id == 2) lighting.rg *= vec2(0.25);
            if(cascade_id == 3) lighting.b  *= 0.125;
            if(interpolation_mode != 0) lighting *= 0.5;
        }
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


void debug_views(in float depth)
{
    vec4 base_color = get_base_color();
    vec3 normal = get_normal();
    vec3 occlusion_roughness_metallic = get_occlusion_roughness_metallic();
    float occlusion_factor = occlusion_roughness_metallic.r;
    float perceptual_roughness = occlusion_roughness_metallic.g;
    float metallic = occlusion_roughness_metallic.b;
    vec3 emissive = get_emissive();

    if(debug_views_position)
        frag_color = vec4(world_space_from_depth(depth, texcoord, inverse_view_projection), 1.0);
    if(debug_views_normal)
        frag_color = vec4(normal, 1.0);
    if(debug_views_depth)
    {
        float z_lin = (2.0 * camera_params.x) / (camera_params.y + camera_params.x - depth * (camera_params.y - camera_params.x));
        frag_color = vec4(vec3(z_lin), 1.0);
    }
    if(debug_views_base_color)
        frag_color = vec4(vec3(1.0 - metallic) * base_color.rgb, 1.0);
    if(debug_views_reflection_color)
        frag_color = vec4(vec3(metallic) * base_color.rgb, 1.0);
    if(debug_views_occlusion)
        frag_color = vec4(vec3(occlusion_factor), 1.0);
    if(debug_views_roughness)
        frag_color = vec4(vec3(perceptual_roughness), 1.0);
    if(debug_views_metallic)
        frag_color = vec4(vec3(metallic), 1.0);
    if(debug_views_emission)
        frag_color = vec4(vec3(emissive), 1.0);
}
