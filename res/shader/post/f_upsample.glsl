#include <../include/common_constants_and_functions.glsl>
#include <../include/bloom_data.glsl>

out vec3 frag_color;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_input; // texture "texture_input"

// see https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
// tent filter

void main()
{
    ivec3 offsets = ivec3(0, -filter_radius, filter_radius);

    vec3 samples[9];

    samples[0] = textureLod(sampler_input, texcoord, current_mip).rgb;

    samples[1] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.xy).rgb;
    samples[2] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yx).rgb;
    samples[3] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.xz).rgb;
    samples[4] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zx).rgb;

    samples[5] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yy).rgb;
    samples[6] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zy).rgb;
    samples[7] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zz).rgb;
    samples[8] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yz).rgb;

    frag_color = 0.25 * samples[0]; // 4.0 / 16.0
    frag_color += 0.125 * (samples[1] + samples[2] + samples[3] + samples[4]); // 2.0 / 16.0
    frag_color += 0.0625 * (samples[5] + samples[6] + samples[7] + samples[8]); // 1.0 / 16.0
}
