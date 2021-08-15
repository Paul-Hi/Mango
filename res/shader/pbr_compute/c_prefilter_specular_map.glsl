
#include <../include/common_constants_and_functions.glsl>
#include <../include/pbr_functions.glsl>

const float width_sqr = 1024.0 * 1024.0; // TODO Paul: Hardcoded -.-
const uint sample_count = 512 - 32;

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0) uniform samplerCube sampler_cubemap_in; // texture "texture_cubemap_in"
layout(binding = 1, rgba16f) uniform writeonly imageCube prefiltered_spec_out;

layout(binding = 3) uniform ibl_generation_data
{
    vec2 out_size;
    vec2 data; // x -> perceptual roughness
};

void main()
{
    ivec3 cube_coords = ivec3(gl_GlobalInvocationID.xyz);
    if (cube_coords.x >= out_size.x || cube_coords.y >= out_size.y)
        return;
    vec3 pos = cube_to_world(cube_coords, out_size);
    vec3 normal = normalize(pos);

    // assume view direction always equal to outgoing direction
    vec3 view = normal;

    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent_x = normalize(cross(up, normal));
    vec3 tangent_y = normalize(cross(normal, tangent_x));

    if (data.x == 0.0) {
        vec4 color = textureLod(sampler_cubemap_in, view, 0);
        imageStore(prefiltered_spec_out, cube_coords, color);
        return;
    }
    float alpha = data.x * data.x;

    vec3 prefiltered = vec3(0.0);
    float weight = 0.0;
    uint real_sample_count = uint(32.0 + float(sample_count) * sqrt(data.x));
    float real_sample_count_inverse = 1.0 / float(real_sample_count);

    for(uint s = 0; s < real_sample_count; ++s)
    {
        vec2 eta  = sample_hammersley(s, real_sample_count_inverse);
        vec3 halfway;
        importance_sample_ggx_direction(eta, normal, tangent_x, tangent_y, alpha, halfway);
        vec3 to_light = normalize(2.0 * dot(view, halfway) * halfway - view);
        float n_dot_l = dot(normal, to_light);

        if(n_dot_l > 0.0)
        {
            float n_dot_h = saturate(dot(normal, halfway));
            float h_dot_v = saturate(dot(halfway, view));
            float pdf = max(D_GGX(n_dot_h, alpha) * n_dot_h / (4.0 * h_dot_v) * INV_PI, 1e-5);
            // formula: eq 13: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
            float o = (1.5 * width_sqr) / (float(real_sample_count) * pdf) * INV_PI;
            float mip_level = max(0.5 * log2(o), 0.0);
            float bias = min(mip_level / 4.0, 0.0); // bias reduces artefacts

            vec3 incoming = textureLod(sampler_cubemap_in, to_light, mip_level + bias).rgb * n_dot_l;
            prefiltered += incoming;
            weight += n_dot_l;
        }
    }

    prefiltered *= (1.0 / weight);

    imageStore(prefiltered_spec_out, cube_coords, vec4(prefiltered, 1.0));
}
