#define COMPUTE
#include <../include/common_constants_and_functions.glsl>
#include <../include/common_pbr.glsl>

const uint sample_count = 512; // sufficient because of the mipmap optimization -> we would need more without!
const float inverse_sample_count = 1.0 / float(sample_count);
const float width_sqr = 1024.0 * 1024.0;

layout(local_size_x = 32, local_size_y = 32) in;

layout(location = 0) uniform samplerCube cubemap_in;
layout(binding = 1, rgba16f) uniform writeonly imageCube irradiance_map_out;

layout(location = 1) uniform vec2 out_size;

void main()
{
    ivec3 cube_coords = ivec3(gl_GlobalInvocationID.xyz);
    if (cube_coords.x >= out_size.x || cube_coords.y >= out_size.y)
        return;
    vec3 pos = cube_to_world(cube_coords, out_size);
    vec3 normal = normalize(pos);

    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent_x = normalize(cross(up, normal));
    vec3 tangent_y = normalize(cross(normal, tangent_x));


    vec3 irradiance = vec3(0.0);
    float luma_sum = 0.0;
    float luma_weight = 0.0;
    // assume view direction always equal to outgoing direction
    vec3 view = normal;

    for(uint s = 0; s < sample_count; ++s)
    {
        vec2 eta  = sample_hammersley(s, inverse_sample_count);
        vec3 to_light;
        importance_sample_cosinus_direction(eta, normal, tangent_x, tangent_y, to_light);

        float n_dot_l = dot(normal, to_light);

        if(n_dot_l > 0.0)
        {
            // cosine weighted hemisphere pdf is cos(theta) / PI
            // assuming average solid angle in the input cube map
            // Omega_p = (4*PI) / (fCubeMapDim * fCubeMapDim * 6)
            // Omega_s = PI / (fSampleCount * cos(theta))
            // formula: eq 13: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
            float o = (1.5 * width_sqr) / (float(sample_count) * n_dot_l);
            float mip_level = max(0.5 * log2(o), 0.0);
            float bias = min(mip_level / 4.0, 1.5); // bias reduces artefacts
            irradiance += textureLod(cubemap_in, to_light, mip_level + bias).rgb * n_dot_l;
        }


        vec3 halfway;
        importance_sample_ggx_direction(eta, normal, tangent_x, tangent_y, 1.0, halfway);
        to_light = normalize(2.0 * dot(view, halfway) * halfway - view);
        n_dot_l = dot(normal, to_light);

        if(n_dot_l > 0.0)
        {
            float n_dot_h = saturate(dot(normal, halfway));
            float h_dot_v = saturate(dot(halfway, view));
            float pdf = max(D_GGX(n_dot_h, 1.0) * n_dot_h / (4.0 * h_dot_v) * INV_PI, 1e-5);
            // formula: eq 13: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
            float o = (1.5 * width_sqr) / (float(sample_count) * pdf) * INV_PI;
            float mip_level = max(0.5 * log2(o), 0.0);
            float bias = min(mip_level / 4.0, 1.5); // bias reduces artefacts

            vec3 incoming = textureLod(cubemap_in, to_light, mip_level + bias).rgb * n_dot_l;
            luma_sum += luma(incoming);
            luma_weight += n_dot_l;
        }
    }

    irradiance *= PI * inverse_sample_count;
    luma_sum *= (1.0 / luma_weight);

    imageStore(irradiance_map_out, cube_coords, vec4(irradiance, luma_sum));
}
