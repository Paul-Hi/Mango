#include <../include/common_constants_and_functions.glsl>

out float ao;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_gtao; // texture "texture_gtao"
layout(binding = 1) uniform sampler2D sampler_depth; // texture "texture_depth"


void main()
{
    vec4[4] ao_q;
    float[9] ao_samples;
    vec4[4] depth_q;
    float[9] depth_samples;
    ivec2 q_offsets = ivec2(-2, 1);
    ivec3 p_offsets = ivec3(0,  1,  2);
    ivec3 n_offsets = ivec3(0, -1, -2);
    ao_q[0] = textureGatherOffset(sampler_gtao, texcoord, q_offsets.xx, 0);
    ao_q[1] = textureGatherOffset(sampler_gtao, texcoord, q_offsets.xy, 0);
    ao_q[2] = textureGatherOffset(sampler_gtao, texcoord, q_offsets.yy, 0);
    ao_q[3] = textureGatherOffset(sampler_gtao, texcoord, q_offsets.yx, 0);
    ao_samples[0] = texture(sampler_gtao, texcoord).x;
    ao_samples[1] = textureOffset(sampler_gtao, texcoord, n_offsets.yx).x;
    ao_samples[2] = textureOffset(sampler_gtao, texcoord, n_offsets.zx).x;
    ao_samples[3] = textureOffset(sampler_gtao, texcoord, p_offsets.yx).x;
    ao_samples[4] = textureOffset(sampler_gtao, texcoord, p_offsets.zx).x;
    ao_samples[5] = textureOffset(sampler_gtao, texcoord, n_offsets.xy).x;
    ao_samples[6] = textureOffset(sampler_gtao, texcoord, n_offsets.xz).x;
    ao_samples[7] = textureOffset(sampler_gtao, texcoord, p_offsets.xy).x;
    ao_samples[8] = textureOffset(sampler_gtao, texcoord, p_offsets.xz).x;

    depth_q[0] = textureGatherOffset(sampler_depth, texcoord, q_offsets.xx, 0);
    depth_q[1] = textureGatherOffset(sampler_depth, texcoord, q_offsets.xy, 0);
    depth_q[2] = textureGatherOffset(sampler_depth, texcoord, q_offsets.yy, 0);
    depth_q[3] = textureGatherOffset(sampler_depth, texcoord, q_offsets.yx, 0);
    depth_samples[0] = texture(sampler_depth, texcoord).x;
    depth_samples[1] = textureOffset(sampler_depth, texcoord, n_offsets.yx).x;
    depth_samples[2] = textureOffset(sampler_depth, texcoord, n_offsets.zx).x;
    depth_samples[3] = textureOffset(sampler_depth, texcoord, p_offsets.yx).x;
    depth_samples[4] = textureOffset(sampler_depth, texcoord, p_offsets.zx).x;
    depth_samples[5] = textureOffset(sampler_depth, texcoord, n_offsets.xy).x;
    depth_samples[6] = textureOffset(sampler_depth, texcoord, n_offsets.xz).x;
    depth_samples[7] = textureOffset(sampler_depth, texcoord, p_offsets.xy).x;
    depth_samples[8] = textureOffset(sampler_depth, texcoord, p_offsets.xz).x;

    float center_depth = depth_samples[0];

    float ao_avg = 0;
    float weight = 0;

    float threshold = abs(center_depth) * 0.1;
    for (int i = 0; i < 9; ++i) {
        float diff = abs(depth_samples[i] - center_depth);
        if (diff < threshold) {
            float sample_weight = 1.0 - saturate(diff / threshold);
            weight += sample_weight;
            ao_avg += ao_samples[i] * sample_weight;
        }
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float diff = abs(depth_q[i][j] - center_depth);
            if (diff < threshold) {
                float sample_weight = 1.0 - saturate(diff / threshold);
                weight += sample_weight;
                ao_avg += ao_q[i][j] * sample_weight;
            }
        }
    }

    ao = ao_avg / max(weight, 1e-5);
}
