#ifndef MANGO_COMMON_SHADOW_GLSL
#define MANGO_COMMON_SHADOW_GLSL

#include <../include/common_constants_and_functions.glsl>
#include <common_state.glsl>

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

const mat4 shadow_bias_matrix = mat4(
                                vec4(0.5,0.0,0.0,0.0),
                                vec4(0.0,0.5,0.0,0.0),
                                vec4(0.0,0.0,0.5,0.0),
                                vec4(0.5,0.5,0.5,1.0)
                                );

// interpolation_mode: 0 -> no interpolation | 1 -> interpolation from cascade_id to cascade_id + 1 - shadow_cascade_interpolation_range | 2 -> interpolation from cascade_id - 1  + shadow_cascade_interpolation_range to cascade_id
int compute_cascade_id(in float view_depth, out float interpolation_factor, out int interpolation_mode) // xy texcoords, z depth
{
    interpolation_mode = 0;
    int cascade_id = 0;
    int num_cascades = get_shadow_cascade_count();
    for(int i = 1; i <= num_cascades; ++i)
    {
        float i_value = get_shadow_cascades_split_depth(i);
        if(view_depth <= i_value)
        {
            cascade_id = i - 1;
            float i_minus_one_value = get_shadow_cascades_split_depth(i - 1);
            float mid = (i_value + i_minus_one_value) * 0.5;
            if(view_depth > mid && view_depth > i_value - get_shadow_cascade_interpolation_range())
            {
                interpolation_mode = 1;
                interpolation_factor = (i_value - view_depth) / get_shadow_cascade_interpolation_range();
            }
            if(view_depth < mid && view_depth < i_minus_one_value + get_shadow_cascade_interpolation_range())
            {
                interpolation_mode = 2;
                interpolation_factor = (view_depth - i_minus_one_value) / get_shadow_cascade_interpolation_range();
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

vec2 sample_blocker(in vec3 shadow_coords, in int cascade_id, in int sample_count)
{
    float avg_blocker_depth = 0.0;
    int blocker_count = 0;
    float search_width = get_shadow_light_size();
    for (int i = 0; i < sample_count; ++i)
    {
        vec2 sample_uv = shadow_coords.xy + poisson_disk[i] * search_width;
        bvec2 outside = greaterThan(sample_uv, vec2(1.0));
        bvec2 inside = lessThan(sample_uv, vec2(0.0));
        if(any(outside) || any(inside))
            continue;
        float z = texture(shadow_map, vec3(sample_uv, cascade_id)).x;
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
    float rd = linearize_depth(receiver_z, 1e-5, get_shadow_cascade_far_planes(cascade_id));
    float bd = linearize_depth(blocker_z, 1e-5, get_shadow_cascade_far_planes(cascade_id));
    return (rd - bd) / bd;
}

float pcf(in vec3 shadow_coords, in int cascade_id, in int sample_count, in float filter_radius)
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
        float z = texture(shadow_map, vec3(sample_uv, cascade_id)).x;
        sum += (z < shadow_coords.z) ? 0.0 : 1.0;
    }

    return sum / sample_count;
}


float pcss(in vec3 shadow_coords, in int cascade_id, in int sample_count)
{
    int num_blocker_samples = int(max(sample_count * 0.25, 16.0));
    vec2 blocker_data = sample_blocker(shadow_coords, cascade_id, num_blocker_samples);
    if(blocker_data.y < 1)
        return 1.0;
    float penumbra_ratio = calculate_penumbra_size(shadow_coords.z, blocker_data.x, cascade_id);
    float filter_radius = penumbra_ratio * get_shadow_light_size();
    filter_radius = max(filter_radius, 2.0 / min(get_shadow_resolution(), 2048));
    return pcf(shadow_coords, cascade_id, sample_count, filter_radius);
}

float calculate_shadow(in vec3 shadow_coords, in int cascade_id)
{
    int num_samples = max(get_shadow_sample_count(), 16);
    switch(get_shadow_filter_mode())
    {
        case 1:
            return pcf(shadow_coords, cascade_id, num_samples, 2.0 / min(get_shadow_resolution(), 2048));
        case 2:
            return pcf(shadow_coords, cascade_id, num_samples, 4.0 / min(get_shadow_resolution(), 2048));
        case 3:
            return pcss(shadow_coords, cascade_id, num_samples);
        case 0:
        default:
            float z = texture(shadow_map, vec3(shadow_coords.xy, cascade_id)).x;
            return (z < shadow_coords.z) ? 0.0 : 1.0;
    }
}

float directional_shadow()
{
    vec3 world_pos = get_world_space_position();
    vec3 normal    = get_normal();

    vec4 view_pos = get_camera_view_matrix() * vec4(world_pos, 1.0);
    int interpolation_mode;
    float interpolation_factor;
    int cascade_id = compute_cascade_id(abs(view_pos.z), interpolation_factor, interpolation_mode);

    vec3 light_dir = normalize(get_directional_light_direction());
    float n_dot_l = saturate(dot(normal, light_dir));
    vec3 bias = vec3(clamp(get_shadow_slope_bias() * tan(acos(n_dot_l)), 0.0, get_shadow_slope_bias() * 2.0)) + normal * get_shadow_normal_bias();

    world_pos += bias;
    float shadow = 1.0;


    if(interpolation_mode == 0)
    {
        vec4 projected = shadow_bias_matrix * get_shadow_camera_view_projection_matrix(cascade_id) * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;

        if(shadow_coords.z > 1.0)
            return shadow;

        shadow = calculate_shadow(shadow_coords, cascade_id);
    }
    else if(interpolation_mode == 1)
    {
        vec4 projected = shadow_bias_matrix * get_shadow_camera_view_projection_matrix(cascade_id) * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = calculate_shadow(shadow_coords, cascade_id);

        vec4 projected2 = shadow_bias_matrix * get_shadow_camera_view_projection_matrix(cascade_id + 1) * vec4(world_pos, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;

        if(shadow_coords2.z > 1.0)
            return calculate_shadow(shadow_coords, cascade_id);

        float pcss_shadows2 = calculate_shadow(shadow_coords2, cascade_id + 1);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }
    else if(interpolation_mode == 2)
    {
        vec4 projected = shadow_bias_matrix * get_shadow_camera_view_projection_matrix(cascade_id) * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = calculate_shadow(shadow_coords, cascade_id);

        vec4 projected2 = shadow_bias_matrix * get_shadow_camera_view_projection_matrix(cascade_id - 1) * vec4(world_pos, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;

        if(shadow_coords2.z > 1.0)
            return calculate_shadow(shadow_coords, cascade_id);

        float pcss_shadows2 = calculate_shadow(shadow_coords2, cascade_id - 1);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }

    return shadow;
}

void draw_shadow_maps_debug(in vec2 uv)
{
    bool sm3 = uv.x > 0.75;
    bool sm2 = uv.x > 0.5;
    bool sm1 = uv.x > 0.25;
    if(sm3)
    {
        uv = vec2((uv.x - 0.75) / 0.25, uv.y / 0.25);
        frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 3)).x), 1.0);
        return;
    }
    if(sm2)
    {
        uv = vec2((uv.x - 0.5) / 0.25, uv.y / 0.25);
        frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 2)).x), 1.0);
        return;
    }
    if(sm1)
    {
        uv = vec2((uv.x - 0.25) / 0.25, uv.y / 0.25);
        frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 1)).x), 1.0);
        return;
    }
    // sm0
    uv = vec2((uv.x) / 0.25, uv.y / 0.25);
    frag_color = vec4(vec3(texture(shadow_map, vec3(uv, 0)).x), 1.0);
}

vec3 get_shadow_cascade_debug_color()
{
    vec3 world_pos = get_world_space_position();
    vec4 view_pos = get_camera_view_matrix() * vec4(world_pos, 1.0);
    int interpolation_mode;
    float interpolation_factor;
    int cascade_id = compute_cascade_id(abs(view_pos.z), interpolation_factor, interpolation_mode);

    vec3 cascade_color = vec3(1.0);
    if(cascade_id == 0) cascade_color.gb *= vec2(0.25);
    if(cascade_id == 1) cascade_color.rb *= vec2(0.25);
    if(cascade_id == 2) cascade_color.rg *= vec2(0.25);
    if(cascade_id == 3) cascade_color.b  *= 0.125;
    if(interpolation_mode != 0) cascade_color *= 0.5;
    return cascade_color;
}

#endif // MANGO_COMMON_SHADOW_GLSL
