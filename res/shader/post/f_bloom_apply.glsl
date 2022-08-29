#include <../include/common_constants_and_functions.glsl>
#include <../include/bloom_data.glsl>

out vec3 frag_color;

in vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_input; // texture "texture_input"
layout(binding = 1) uniform sampler2D sampler_lens; // texture "texture_lens"

vec3 last_upsample()
{
    vec3 bloom_color;
    ivec3 offsets = ivec3(0, -filter_radius, filter_radius);

    vec3 samples[9];

    samples[0] = textureLod(sampler_input, texcoord, 1).rgb;

    samples[1] = textureLodOffset(sampler_input, texcoord, 1, offsets.xy).rgb;
    samples[2] = textureLodOffset(sampler_input, texcoord, 1, offsets.yx).rgb;
    samples[3] = textureLodOffset(sampler_input, texcoord, 1, offsets.xz).rgb;
    samples[4] = textureLodOffset(sampler_input, texcoord, 1, offsets.zx).rgb;

    samples[5] = textureLodOffset(sampler_input, texcoord, 1, offsets.yy).rgb;
    samples[6] = textureLodOffset(sampler_input, texcoord, 1, offsets.zy).rgb;
    samples[7] = textureLodOffset(sampler_input, texcoord, 1, offsets.zz).rgb;
    samples[8] = textureLodOffset(sampler_input, texcoord, 1, offsets.yz).rgb;

    bloom_color = 0.25 * samples[0]; // 4.0 / 16.0
    bloom_color += 0.125 * (samples[1] + samples[2] + samples[3] + samples[4]); // 2.0 / 16.0
    bloom_color += 0.0625 * (samples[5] + samples[6] + samples[7] + samples[8]); // 1.0 / 16.0
    return bloom_color;
}

void main()
{
    vec3 hdr_color = textureLod(sampler_input, texcoord, 0).rgb;
    vec3 bloom_color = hdr_color + last_upsample();
    if (lens_texture)
        bloom_color *= (1.0 + lens_texture_intensity * texture(sampler_lens, vec2(texcoord.x, 1.0 - texcoord.y)).rgb);
    frag_color = mix(hdr_color, bloom_color, power);
}
