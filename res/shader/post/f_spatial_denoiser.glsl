#include <../include/common_constants_and_functions.glsl>

out float ao;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_gtao; // texture "texture_gtao"
layout(binding = 1) uniform sampler2D sampler_depth; // texture "texture_depth"


void main()
{
    vec4[4] ao_q;
    vec4[4] depth_q;
    ao_q[0] = textureGather(sampler_gtao, texcoord);
	ao_q[1] = textureGatherOffset(sampler_gtao, texcoord, ivec2(0,-2));
	ao_q[2] = textureGatherOffset(sampler_gtao, texcoord, ivec2(-2,0));
	ao_q[3] = textureGatherOffset(sampler_gtao, texcoord, ivec2(-2,-2));
    depth_q[0] = textureGather(sampler_depth, texcoord);
	depth_q[1] = textureGatherOffset(sampler_depth, texcoord, ivec2(0,-2));
	depth_q[2] = textureGatherOffset(sampler_depth, texcoord, ivec2(-2,0));
	depth_q[3] = textureGatherOffset(sampler_depth, texcoord, ivec2(-2,-2));

    float center_depth = depth_q[0][3];

    float ao_avg = 0;
    float weight = 0;

    float threshold = abs(center_depth) * 0.1;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float diff = abs(depth_q[i][j] - center_depth);
            if (diff < threshold) {
                float sample_weight = 1.0 - saturate(10.0 * diff / threshold);
                weight += sample_weight;
                ao_avg += ao_q[i][j] * sample_weight;
            }
        }
    }

    ao = ao_avg / max(weight, 1e-5);
}
