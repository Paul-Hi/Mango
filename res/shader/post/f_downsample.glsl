#include <../include/common_constants_and_functions.glsl>
#include <../include/bloom_data.glsl>

out vec3 frag_color;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_input; // texture "texture_input"

// see https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare

float karis_average(in vec3 color)
{
    float luma = luma(linear_to_srgb(color));
    return 1.0 / (1.0 + luma);
}

void main()
{
    ivec3 offsets = ivec3(0, -1, 1);

    vec3 samples[13];

    samples[0] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yy).rgb;
    samples[1] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zy).rgb;
    samples[2] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zz).rgb;
    samples[3] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yz).rgb;

    samples[4] = textureLod(sampler_input, texcoord, current_mip).rgb;
    samples[5] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.xy * 2).rgb;
    samples[6] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yx * 2).rgb;
    samples[7] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yy * 2).rgb;
    samples[8] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zy * 2).rgb;
    samples[9] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zx * 2).rgb;
    samples[10] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.zz * 2).rgb;
    samples[11] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.xz * 2).rgb;
    samples[12] = textureLodOffset(sampler_input, texcoord, current_mip, offsets.yz * 2).rgb;

    if (apply_karis) // (first downsample)
    {
        vec3 groups[5];
        groups[0] = (samples[0] + samples[1] + samples[2] + samples[3]) * 0.125; // 0.5 * 0.25
        groups[1] = (samples[4] + samples[5] + samples[6] + samples[7]) * 0.03125; // 0.125 * 0.25
        groups[2] = (samples[4] + samples[5] + samples[8] + samples[9]) * 0.03125; // 0.125 * 0.25
        groups[3] = (samples[4] + samples[9] + samples[10] + samples[11]) * 0.03125; // 0.125 * 0.25
        groups[4] = (samples[4] + samples[6] + samples[11] + samples[12]) * 0.03125; // 0.125 * 0.25
        groups[0] *= karis_average(groups[0]);
        groups[1] *= karis_average(groups[1]);
        groups[2] *= karis_average(groups[2]);
        groups[3] *= karis_average(groups[3]);
        groups[4] *= karis_average(groups[4]);
        frag_color = (groups[0] + groups[1] + groups[2] + groups[3] + groups[4]);
    }
    else
    {
        frag_color = 0.125 * (samples[0] + samples[1] + samples[2] + samples[3] + samples[4]); // 0.5 * 0.25
        frag_color += 0.0625 * (samples[5] + samples[6] + samples[9] + samples[11]); // 0.25 * 0.25
        frag_color += 0.03125 * (samples[7] + samples[8] + samples[10] + samples[12]); // 0.125 * 0.25
    }

}
