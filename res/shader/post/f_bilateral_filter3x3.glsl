#include <../include/common_constants_and_functions.glsl>

out float ao;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_gtao; // texture "texture_gtao"
layout(binding = 1) uniform sampler2D sampler_depth; // texture "texture_depth"


void main()
{
    vec4 ao_q;
    float[5] ao_samples;
    vec4 depth_q;
    float[5] depth_samples;
    ivec3 offsets = ivec3(0, -1, 1);
    ao_q = textureGather(sampler_gtao, texcoord, 0);
    ao_samples[0] = textureOffset(sampler_gtao, texcoord, offsets.yz).x;
    ao_samples[1] = textureOffset(sampler_gtao, texcoord, offsets.yx).x;
    ao_samples[2] = textureOffset(sampler_gtao, texcoord, offsets.yy).x;
    ao_samples[3] = textureOffset(sampler_gtao, texcoord, offsets.xy).x;
    ao_samples[4] = textureOffset(sampler_gtao, texcoord, offsets.zy).x;
    depth_q = textureGather(sampler_depth, texcoord, 0);
    depth_samples[0] = textureOffset(sampler_depth, texcoord, offsets.yz).x;
    depth_samples[1] = textureOffset(sampler_depth, texcoord, offsets.yx).x;
    depth_samples[2] = textureOffset(sampler_depth, texcoord, offsets.yy).x;
    depth_samples[3] = textureOffset(sampler_depth, texcoord, offsets.xy).x;
    depth_samples[4] = textureOffset(sampler_depth, texcoord, offsets.zy).x;

    float center_depth = depth_q.w;

    float ao_avg = 0;
    float weight = 0;

    float threshold = abs(center_depth) * 0.1;
    for (int i = 0; i < 5; ++i) {
        float diff = abs(depth_samples[i] - center_depth);
        if (diff < threshold) {
            float sample_weight = 1.0 - saturate(diff / threshold);
            weight += sample_weight;
            ao_avg += ao_samples[i] * sample_weight;
        }
    }
    for (int i = 0; i < 4; ++i) {
        float diff = abs(depth_q[i] - center_depth);
        if (diff < threshold) {
            float sample_weight = 1.0 - saturate(diff / threshold);
            weight += sample_weight;
            ao_avg += ao_q[i] * sample_weight;
        }
    }

    ao = ao_avg / max(weight, 1e-5);
}
