#ifndef MANGO_SHADOW_FUNCTIONS_GLSL
#define MANGO_SHADOW_FUNCTIONS_GLSL

#include <shadow.glsl>
#include <camera.glsl>
#include <common_constants_and_functions.glsl>

// interpolation_mode: 0 -> no interpolation | 1 -> interpolation from cascade_id to cascade_id + 1 - shadow_cascade_interpolation_range | 2 -> interpolation from cascade_id - 1  + shadow_cascade_interpolation_range to cascade_id
int compute_cascade_id(in float view_depth, out float interpolation_factor, out int interpolation_mode) // xy texcoords, z depth
{
    interpolation_mode = 0;
    int cascade_id = 0;
    for(int i = 0; i < shadow_cascade_count; ++i)
    {
        float i_value = split_depth[i];
        if(view_depth < i_value)
        {
            cascade_id = i;
            float i_minus_one_value = i > 0 ? split_depth[i - 1] : camera_near;
            float mid = (i_value + i_minus_one_value) * 0.5;
            if(view_depth > mid && view_depth > i_value - shadow_cascade_interpolation_range)
            {
                interpolation_mode = 1;
                interpolation_factor = (i_value - view_depth) / shadow_cascade_interpolation_range;
            }
            if(view_depth < mid && view_depth < i_minus_one_value + shadow_cascade_interpolation_range)
            {
                interpolation_mode = 2;
                interpolation_factor = (view_depth - i_minus_one_value) / shadow_cascade_interpolation_range;
            }
            break;
        }
    }

    interpolation_factor = (1.0 - interpolation_factor);

    if(cascade_id == 0 && interpolation_mode == 2)
        interpolation_mode = 0;
    if(cascade_id == shadow_cascade_count - 1 && interpolation_mode == 1)
        interpolation_mode = 0;

    return cascade_id;
}

vec2 sample_blocker(in vec2 shadow_uv, in float receiver_z, in int cascade_id, in int sample_count, in float search_radius)
{
    float average_depth = 0.0;
    int blocker_count = 0;
    float theta = interleaved_gradient_noise(gl_FragCoord.xy);
    mat2 rotation = mat2(vec2(cos(theta), sin(theta)), vec2(-sin(theta), cos(theta)));
    for(int i = 0; i < sample_count; ++i)
    {
        vec2 sample_uv = shadow_uv + vogel_disc_sample(i, sample_count, rotation) * search_radius;
        bvec2 outside = greaterThan(sample_uv, vec2(1.0)) || lessThan(sample_uv, vec2(0.0));
        if(any(outside))
            continue;

        float z = texture(sampler_shadow_map, vec3(sample_uv, cascade_id)).x;
        if (z < receiver_z)
        {
            average_depth += z;
            blocker_count++;
        }
    }

    return vec2(average_depth / blocker_count, blocker_count);
}

float pcf(in vec2 shadow_uv, in float receiver_z, in int cascade_id, in int sample_count, in float filter_radius)
{
    float sum = 0.0;
    float theta = interleaved_gradient_noise(gl_FragCoord.xy);
    mat2 rotation = mat2(vec2(cos(theta), sin(theta)), vec2(-sin(theta), cos(theta)));
    for(int i = 0; i < sample_count; ++i)
    {
        vec2 sample_uv = shadow_uv + vogel_disc_sample(i, sample_count, rotation) * filter_radius;
        bvec2 outside = greaterThan(sample_uv, vec2(1.0)) || lessThan(sample_uv, vec2(0.0));
        if(any(outside))
        {
            sum += 1.0;
            continue;
        }
        sum += texture(sampler_shadow_shadow_map, vec4(sample_uv, cascade_id, receiver_z));
    }

    return sum / sample_count;
}

float pcss(in vec2 shadow_uv, in float receiver_z, in int cascade_id, in int sample_count)
{
    vec2 blocker_data = sample_blocker(shadow_uv, receiver_z, cascade_id, sample_count, shadow_light_size / shadow_resolution);
    if(blocker_data.y < 1)
        return 1.0;

    float rd = linearize_depth(receiver_z, 1e-5, shadow_far_planes[cascade_id]);
    float bd = linearize_depth(blocker_data.x, 1e-5, shadow_far_planes[cascade_id]);
    float penumbra_size = clamp(shadow_light_size * (rd - bd) / bd, 1.0, 16.0); // TODO Paul: Clamp Values?
    return pcf(shadow_uv, receiver_z, cascade_id, shadow_sample_count, penumbra_size / shadow_resolution);
}

float calculate_shadow(in vec3 shadow_coords, in int cascade_id)
{
    float receiver_z = shadow_coords.z;
    switch(shadow_filter_mode)
    {
        case 1:
            return pcf(shadow_coords.xy, receiver_z, cascade_id, shadow_sample_count, shadow_width / shadow_resolution);
        case 2:
            return pcss(shadow_coords.xy, receiver_z, cascade_id, shadow_sample_count);
        case 0:
        default:
            return texture(sampler_shadow_shadow_map, vec4(shadow_coords.xy, cascade_id, receiver_z));
    }
}

float directional_shadow(in vec3 world_position, in vec3 normal)
{
    vec4 view_pos = view_matrix * vec4(world_position, 1.0);
    int interpolation_mode;
    float interpolation_factor;
    int cascade_id = compute_cascade_id(abs(view_pos.z), interpolation_factor, interpolation_mode);

    vec3 light_dir = normalize(directional_direction.xyz);
    float n_dot_l = saturate(dot(normal, light_dir));
    vec3 bias = vec3(clamp(shadow_slope_bias * tan(acos(n_dot_l)), 0.0, shadow_slope_bias * 2.0)) + normal * shadow_normal_bias;

    world_position += bias;
    float shadow = 1.0;

    if(interpolation_mode == 0)
    {
        vec4 projected = shadow_view_projection_matrices[cascade_id] * vec4(world_position, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;
        shadow_coords = shadow_coords * 0.5 + 0.5;

        if(shadow_coords.z > 1.0)
            return shadow;

        shadow = calculate_shadow(shadow_coords, cascade_id);
    }
    else if(interpolation_mode == 1)
    {
        vec4 projected = shadow_view_projection_matrices[cascade_id] * vec4(world_position, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;
        shadow_coords = shadow_coords * 0.5 + 0.5;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = calculate_shadow(shadow_coords, cascade_id);

        vec4 projected2 = shadow_view_projection_matrices[cascade_id + 1] * vec4(world_position, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;
        shadow_coords2 = shadow_coords2 * 0.5 + 0.5;

        if(shadow_coords2.z > 1.0)
            return calculate_shadow(shadow_coords, cascade_id);

        float pcss_shadows2 = calculate_shadow(shadow_coords2, cascade_id + 1);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }
    else if(interpolation_mode == 2)
    {
        vec4 projected = shadow_view_projection_matrices[cascade_id] * vec4(world_position, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;
        shadow_coords = shadow_coords * 0.5 + 0.5;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = calculate_shadow(shadow_coords, cascade_id);

        vec4 projected2 = shadow_view_projection_matrices[cascade_id - 1] * vec4(world_position, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;
        shadow_coords2 = shadow_coords2 * 0.5 + 0.5;

        if(shadow_coords2.z > 1.0)
            return calculate_shadow(shadow_coords, cascade_id);

        float pcss_shadows2 = calculate_shadow(shadow_coords2, cascade_id - 1);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }

    return shadow;
}

vec3 get_shadow_cascade_debug_color(in vec3 world_position)
{
    float interpolation_factor;
    int interpolation_mode;
    vec4 view_pos = view_matrix * vec4(world_position, 1.0);
    int cascade_id = compute_cascade_id(abs(view_pos.z), interpolation_factor, interpolation_mode);

    vec3 cascade_color = vec3(1.0);
    if(cascade_id == 0) cascade_color.gb *= vec2(0.25);
    if(cascade_id == 1) cascade_color.rb *= vec2(0.25);
    if(cascade_id == 2) cascade_color.rg *= vec2(0.25);
    if(cascade_id == 3) cascade_color.b  *= 0.125;
    if(interpolation_mode != 0) cascade_color *= 0.5;
    return cascade_color;
}

#endif // MANGO_SHADOW_FUNCTIONS_GLSL
