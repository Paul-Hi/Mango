#include <../include/common_constants_and_functions.glsl>

out vec4 frag_color;

in noperspective vec2 texcoord;

layout(location = 0) uniform sampler2D input_texture;
layout(location = 1) uniform vec2 inverse_screen_size;
layout(location = 2) uniform int quality_preset;
layout(location = 3) uniform float subpixel_filter;

#define FXAA_SEARCH_THRESHOLD 1.0 / 4.0

#define LOW_Q_FXAA_EDGE_THRESHOLD 1.0 / 4.0
#define HIGH_Q_FXAA_EDGE_THRESHOLD 1.0 / 8.0

#define FXAA_EDGE_THRESHOLD_MIN 1.0 / 16.0

#define FXAA_SEARCH_STEPS_LOW 4
#define FXAA_SEARCH_STEPS_DEFAULT 6
#define FXAA_SEARCH_STEPS_HIGH 7

struct quality
{
    float edge_threshold;
    float edge_threshold_min;
    int search_steps;
    float step_sizes[FXAA_SEARCH_STEPS_HIGH];
};

const quality quality_settings[3] = {
    { LOW_Q_FXAA_EDGE_THRESHOLD, FXAA_EDGE_THRESHOLD_MIN, FXAA_SEARCH_STEPS_LOW,
    { 1.0, 1.5, 3.0, 12.0, 0.0, 0.0, 0.0 } },
    { HIGH_Q_FXAA_EDGE_THRESHOLD, FXAA_EDGE_THRESHOLD_MIN, FXAA_SEARCH_STEPS_DEFAULT,
    { 1.0, 1.5, 2.0, 2.0, 4.0, 12.0, 0.0 } },
    { HIGH_Q_FXAA_EDGE_THRESHOLD, FXAA_EDGE_THRESHOLD_MIN, FXAA_SEARCH_STEPS_HIGH,
    { 1.0, 1.5, 2.0, 2.0, 2.0, 3.0, 8.0 } },
};

void main()
{
    quality q = quality_settings[quality_preset];

    vec4 rgba_center  = textureLod(input_texture, texcoord, 0.0);

    float luma_center = luma(rgba_center);
    float luma_north  = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2( 0, -1)));
    float luma_west   = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2(-1,  0)));
    float luma_east   = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2( 1,  0)));
    float luma_south  = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2( 0,  1)));
    float min_luma    = min(luma_center, min(min(luma_west, luma_east), min(luma_north, luma_south)));
    float max_luma    = max(luma_center, max(max(luma_west, luma_east), max(luma_north, luma_south)));
    float luma_range  = max_luma - min_luma;

    if(luma_range < max(q.edge_threshold_min, max_luma * q.edge_threshold))
    {
        frag_color = rgba_center;
        return;
    }

    float luma_north_west = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2(-1, -1)));
    float luma_north_east = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2( 1, -1)));
    float luma_south_west = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2(-1,  1)));
    float luma_south_east = luma(textureLodOffset(input_texture, texcoord, 0.0, ivec2( 1,  1)));


    float vertical =
        abs((0.25 * luma_north_west) + (-0.5 * luma_north) + (0.25 * luma_north_east)) +
        abs((0.50 * luma_west ) + (-1.0 * luma_center) + (0.50 * luma_east )) +
        abs((0.25 * luma_south_west) + (-0.5 * luma_south) + (0.25 * luma_south_east));
    float horizontal =
        abs((0.25 * luma_north_west) + (-0.5 * luma_west) + (0.25 * luma_south_west)) +
        abs((0.50 * luma_north ) + (-1.0 * luma_center) + (0.50 * luma_south )) +
        abs((0.25 * luma_north_east) + (-0.5 * luma_east) + (0.25 * luma_south_east));


    bool horizontal_span = horizontal >= vertical;

    // if(horizontal_span)
    // {
    //     frag_color = vec4(0.0, 1.0, 0.0, 1.0);
    //     return;
    // }
    // else
    // {
    //     frag_color = vec4(1.0, 0.0, 0.0, 1.0);
    //     return;
    // }

    // opposite lumas
    float luma0 = horizontal_span ? luma_north : luma_west;
    float luma1 = horizontal_span ? luma_south : luma_east;


    float length_sign = horizontal_span ? inverse_screen_size.y : inverse_screen_size.x;


    float gradient0 = abs(luma0 - luma_center);
    float gradient1 = abs(luma1 - luma_center);

    bool steeper_on0 = gradient0 >= gradient1;
    float scaled_gradient = FXAA_SEARCH_THRESHOLD * max(gradient0, gradient1);

    float luma0c = (luma0 + luma_center);
    float luma1c = (luma1 + luma_center);

    if(steeper_on0) // switch direction
        length_sign = -length_sign;
    else // swutch luma
        luma0c = luma1c;

    float half_luma0c = luma0c * 0.5;

    vec2 length_sign_vec = vec2(length_sign * 0.5,  0.0);
    vec2 sample_uv = texcoord + (horizontal_span ? length_sign_vec.yx : length_sign_vec.xy);
    vec2 offset = horizontal_span ? vec2(inverse_screen_size.x, 0.0) : vec2(0.0, inverse_screen_size.y) ;

    vec2 sample_dir0 = sample_uv - (offset * q.step_sizes[0]);
    vec2 sample_dir1 = sample_uv + (offset * q.step_sizes[0]);

    float luma_end0 = luma(textureLod(input_texture, sample_dir0, 0.0)) - half_luma0c;
    float luma_end1 = luma(textureLod(input_texture, sample_dir1, 0.0)) - half_luma0c;

    bool done0 = (abs(luma_end0) >= scaled_gradient);
    bool done1 = (abs(luma_end1) >= scaled_gradient);

    if(!done0) sample_dir0 -= offset * q.step_sizes[1];
    if(!done1) sample_dir1 += offset * q.step_sizes[1];
    bool not_done = true;

    for(int i = 2; i < q.search_steps; ++i)
    {
        if(!done0) luma_end0 = luma(textureLod(input_texture, sample_dir0, 0.0)) - half_luma0c;
        if(!done1) luma_end1 = luma(textureLod(input_texture, sample_dir1, 0.0)) - half_luma0c;

        done0 = done0 || (abs(luma_end0) >= scaled_gradient);
        done1 = done1 || (abs(luma_end1) >= scaled_gradient);

        if(!done0) sample_dir0 -= offset * q.step_sizes[i];
        if(!done1) sample_dir1 += offset * q.step_sizes[i];

        not_done = (!done0) || (!done1);
        if(!not_done) break;
    }

    float direction0 = horizontal_span ? texcoord.x - sample_dir0.x : texcoord.y - sample_dir0.y;
    float direction1 = horizontal_span ? sample_dir1.x - texcoord.x : sample_dir1.y - texcoord.y;

    float span_length = (direction0 + direction1);
    float inverse_span_length = 1.0 / span_length;

    bool end0_is_closer = direction0 < direction1;
    float distance_to_end = min(direction0, direction1);
    float pixel_offset = (distance_to_end * (-inverse_span_length)) + 0.5;

    float local_average_luma = luma_center - (luma0c * 0.5);
    bool average_lt_zero = local_average_luma < 0.0;

    bool is_valid_correction = (((end0_is_closer ? luma_end0 : luma_end1) < 0.0) != average_lt_zero);

    float final_offset = is_valid_correction ? pixel_offset : 0.0;

    if(subpixel_filter > 1e-5)
    {
        float full_average_luma =
            (1.0 / 12.0) *
            (2.0 * (luma_north + luma_south + luma_west + luma_east)
            + (luma_north_west + luma_north_east + luma_south_west + luma_south_east));

        float subpixel_offset0 = saturate(abs(full_average_luma - luma_center) / luma_range);
        float subpixel_offset1 = ((-2.0 * subpixel_offset0) + 3.0) * (subpixel_offset0 * subpixel_offset0);
        float subpixel_offset = subpixel_offset1 * subpixel_filter;

        final_offset = max(pixel_offset, subpixel_offset);
    }

    final_offset *= length_sign;
    vec2 final_offset_vec = vec2(final_offset, 0.0);
    vec2 final_uv = texcoord + (horizontal_span ? final_offset_vec.yx : final_offset_vec.xy);

    frag_color = vec4(textureLod(input_texture, final_uv, 0.0).rgb, 1.0);
}
