#include <../include/common_constants_and_functions.glsl>

out vec4 frag_color;

in vec2 texcoord;

layout(location = 0) uniform sampler2D input_texture;
layout(location = 1) uniform vec2 inverse_screen_size;
layout(location = 2) uniform int quality_preset;
layout(location = 3) uniform float subpixel_filter;


#define LOW_Q_FXAA_EDGE_THRESHOLD 1.0 / 4.0
#define HIGH_Q_FXAA_EDGE_THRESHOLD 1.0 / 8.0

#define FXAA_EDGE_THRESHOLD_MIN 1.0 / 16.0

#define FXAA_SEARCH_STEPS_LOW 4
#define FXAA_SEARCH_STEPS_DEFAULT 8
#define FXAA_SEARCH_STEPS_HIGH 12

struct quality
{
    float edge_threshold;
    float edge_threshold_min;
    int search_steps;
    float step_sizes[FXAA_SEARCH_STEPS_HIGH];
};

const quality quality_settings[3] = {
    { LOW_Q_FXAA_EDGE_THRESHOLD, FXAA_EDGE_THRESHOLD_MIN, FXAA_SEARCH_STEPS_LOW,
    { 1.0, 1.5, 2.0, 8.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0 } },
    { HIGH_Q_FXAA_EDGE_THRESHOLD, FXAA_EDGE_THRESHOLD_MIN, FXAA_SEARCH_STEPS_DEFAULT,
    { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0, -1.0, -1.0, -1.0, -1.0 } },
    { HIGH_Q_FXAA_EDGE_THRESHOLD, FXAA_EDGE_THRESHOLD_MIN, FXAA_SEARCH_STEPS_HIGH,
    { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 } },
};

void main()
{
    quality q = quality_settings[quality_preset];

    vec4 rgba_center  = texture(input_texture, texcoord);

    float luma_center = luma(rgba_center);
    float luma_north  = luma(textureOffset(input_texture, texcoord, ivec2( 0, 1)));
    float luma_west   = luma(textureOffset(input_texture, texcoord, ivec2(-1, 0)));
    float luma_east   = luma(textureOffset(input_texture, texcoord, ivec2( 1, 0)));
    float luma_south  = luma(textureOffset(input_texture, texcoord, ivec2( 0,-1)));
    float min_luma    = min(luma_center, min(min(luma_west, luma_east), min(luma_north, luma_south)));
    float max_luma    = max(luma_center, max(max(luma_west, luma_east), max(luma_north, luma_south)));
    float luma_range  = max_luma - min_luma;

    if(luma_range < max(q.edge_threshold_min, max_luma * q.edge_threshold))
    {
        frag_color = rgba_center;
        return;
    }

    float luma_north_west = luma(textureOffset(input_texture, texcoord, ivec2(-1,  1)));
    float luma_north_east = luma(textureOffset(input_texture, texcoord, ivec2( 1,  1)));
    float luma_south_west = luma(textureOffset(input_texture, texcoord, ivec2(-1, -1)));
    float luma_south_east = luma(textureOffset(input_texture, texcoord, ivec2( 1, -1)));

    float vertical =
        abs((0.25 * luma_north_west) + (-0.5 * luma_north) + (0.25 * luma_north_east)) +
        abs((0.50 * luma_west ) + (-1.0 * luma_center) + (0.50 * luma_east )) +
        abs((0.25 * luma_south_west) + (-0.5 * luma_south) + (0.25 * luma_south_east));
    float horizontal =
        abs((0.25 * luma_north_west) + (-0.5 * luma_west) + (0.25 * luma_south_west)) +
        abs((0.50 * luma_north ) + (-1.0 * luma_center) + (0.50 * luma_south )) +
        abs((0.25 * luma_north_east) + (-0.5 * luma_east) + (0.25 * luma_south_east));


    bool horizontal_span = horizontal >= vertical;
    // opposite lumas
    float luma0 = horizontal_span ? luma_south : luma_west;
    float luma1 = horizontal_span ? luma_north : luma_east;


    float step_size = horizontal_span ? inverse_screen_size.y : inverse_screen_size.x;


    float gradient0 = abs(luma0 - luma_center);
    float gradient1 = abs(luma1 - luma_center);

    bool steeper_on0 = gradient0 >= gradient1;
    float scaled_gradient = 0.25 * max(gradient0, gradient1);

    float local_average_luma = 0.5 * (luma1 + luma_center);

    if(steeper_on0) // switch direction
    {
        step_size = -step_size;
        local_average_luma = 0.5 * (luma0 + luma_center);
    }

    vec2 sample_uv = texcoord + (horizontal_span ? vec2(0.0, step_size * 0.5) : vec2(step_size * 0.5,  0.0));
    vec2 offset = horizontal_span ? vec2(inverse_screen_size.x, 0.0) : vec2(0.0, inverse_screen_size.y);

    vec2 sample_dir0 = sample_uv - offset * q.step_sizes[0];
    vec2 sample_dir1 = sample_uv + offset * q.step_sizes[0];

    float luma_end0 = luma(texture(input_texture, sample_dir0)) - local_average_luma;
    float luma_end1 = luma(texture(input_texture, sample_dir1)) - local_average_luma;

    bool done0 = abs(luma_end0) >= scaled_gradient;
    bool done1 = abs(luma_end1) >= scaled_gradient;

    if(!done0) sample_dir0 -= offset * q.step_sizes[1];
    if(!done1) sample_dir1 += offset * q.step_sizes[1];

    for(int i = 2; i < q.search_steps; ++i)
    {
        if(!done0) luma_end0 = luma(texture(input_texture, sample_dir0)) - local_average_luma;
        if(!done1) luma_end1 = luma(texture(input_texture, sample_dir1)) - local_average_luma;

        done0 = done0 || (abs(luma_end0) >= scaled_gradient);
        done1 = done1 || (abs(luma_end1) >= scaled_gradient);

        if(done0 && done1) break;

        if(!done0) sample_dir0 -= offset * q.step_sizes[i];
        if(!done1) sample_dir1 += offset * q.step_sizes[i];
    }

    float direction0 = horizontal_span ? sample_uv.x - sample_dir0.x : sample_uv.y - sample_dir0.y;
    float direction1 = horizontal_span ? sample_dir1.x - sample_uv.x : sample_dir1.y - sample_uv.y;

    float span_length = (direction0 + direction1);
    float inverse_span_length = 1.0 / span_length;

    bool end0_is_closer = direction0 < direction1;
    float distance_to_end = end0_is_closer ? direction0 : direction1;
    float pixel_offset = -distance_to_end * inverse_span_length + 0.5;

    bool center_luma_smaller_average = luma_center < local_average_luma;

    bool is_valid_correction = ((end0_is_closer ? luma_end0 : luma_end1) < 0.0) != center_luma_smaller_average;

    float final_offset = is_valid_correction ? pixel_offset : 0.0;

    if(subpixel_filter > 1e-5)
    {
        float full_average_luma =
            (1.0 / 12.0) *
            (2.0 * (luma_north + luma_south + luma_west + luma_east)
            + (luma_north_west + luma_north_east + luma_south_west + luma_south_east));

        float subpixel_offset0 = clamp(abs(full_average_luma - luma_center) / luma_range, 0.0, 1.0);
        float subpixel_offset1 = (-2.0 * subpixel_offset0 + 3.0) * subpixel_offset0 * subpixel_offset0;
        float subpixel_offset = subpixel_offset1 * subpixel_offset1 * subpixel_filter;

        final_offset = max(pixel_offset, subpixel_offset);
    }

    final_offset *= step_size;
    vec2 final_uv = texcoord + (horizontal_span ? vec2(0.0, final_offset) : vec2(final_offset,  0.0));

    frag_color = vec4(texture(input_texture, final_uv).rgb, 1.0);
}