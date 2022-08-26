#include <../include/common_constants_and_functions.glsl>

out float ao;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_gtao_linear; // texture "texture_gtao_linear"
layout(binding = 1) uniform sampler2D sampler_gtao_nearest; // texture "texture_gtao_nearest"
layout(binding = 2) uniform sampler2D sampler_depth; // texture "texture_depth"


void main()
{
    vec4 depth_samples;
    ivec2 offsets = ivec2(0, 1);
    depth_samples[0] = textureLodOffset(sampler_depth, texcoord, 1, offsets.xx).x;
    depth_samples[1] = textureLodOffset(sampler_depth, texcoord, 1, offsets.xy).x;
    depth_samples[2] = textureLodOffset(sampler_depth, texcoord, 1, offsets.yx).x;
    depth_samples[3] = textureLodOffset(sampler_depth, texcoord, 1, offsets.yy).x;

    float high_res_depth = textureLod(sampler_depth, texcoord, 0).x;

    vec4 depth_diffs;
    depth_diffs[0] = abs(depth_samples[0] - high_res_depth);
    depth_diffs[1] = abs(depth_samples[1] - high_res_depth);
    depth_diffs[2] = abs(depth_samples[2] - high_res_depth);
    depth_diffs[3] = abs(depth_samples[3] - high_res_depth);


    float threshold = abs(high_res_depth) * 0.1;
    if (depth_diffs[0] < threshold &&
        depth_diffs[1] < threshold &&
        depth_diffs[2] < threshold &&
        depth_diffs[3] < threshold)
    {
        ao = textureOffset(sampler_gtao_linear, texcoord, offsets.xx).x;
        ao += textureOffset(sampler_gtao_linear, texcoord, offsets.xy).x;
        ao += textureOffset(sampler_gtao_linear, texcoord, offsets.yx).x;
        ao += textureOffset(sampler_gtao_linear, texcoord, offsets.yy).x;
        ao *= 0.25;
    }
    else
    {
        float depth_min = min(min(depth_diffs[0], depth_diffs[1]), min(depth_diffs[2], depth_diffs[3]));
        ivec2 nearest_depth_offset;

        if (depth_min == depth_diffs[0])
            nearest_depth_offset = offsets.xx;
        else if (depth_min == depth_diffs[1])
            nearest_depth_offset = offsets.xy;
        else if (depth_min == depth_diffs[2])
            nearest_depth_offset = offsets.yx;
        else
            nearest_depth_offset = offsets.yy;

        ao = textureOffset(sampler_gtao_nearest, texcoord, nearest_depth_offset).x;
    }
}
