#define COMPUTE
#include <../include/common_constants_and_functions.glsl>
#include <../include/common_pbr.glsl>

const float width_sqr = 1024.0 * 1024.0; // TODO Paul: Hardcoded -.-
const uint sample_count = 512;

layout(local_size_x = 32, local_size_y = 32) in;

layout(location = 0) uniform samplerCube cubemap_in;
layout(binding = 1, rgba16f) uniform writeonly imageCube prefiltered_spec_out;

layout(location = 1) uniform vec2 out_size;
layout(location = 2) uniform float roughness;

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

    if (roughness == 0.0) {
        vec4 color = textureLod(cubemap_in, view, 0);
        imageStore(prefiltered_spec_out, cube_coords, color);
        return;
    }

    vec3 prefiltered = vec3(0.0);
    float weight = 0.0;
    uint real_sample_count = uint(1.0 + float(sample_count) * roughness);
    float real_sample_count_inverse = 1.0 / float(real_sample_count);

    for(uint s = 0; s < real_sample_count; ++s)
    {
        vec2 eta  = sample_hammersley(s, real_sample_count_inverse);
        vec3 halfway;
        vec3 to_light;
        float n_dot_l;
        importance_sample_ggx_direction(eta, view, normal, tangent_x, tangent_y, roughness, n_dot_l, halfway, to_light);
        n_dot_l = saturate(n_dot_l);

        if(n_dot_l > 0.0)
        {
            float n_dot_h = saturate(dot(normal, halfway));
            float h_dot_v = saturate(dot(halfway, view));
            float pdf = max(D_GGX(n_dot_h, roughness * roughness) * n_dot_h / (PI * 4.0 * h_dot_v), 1e-5);
            float omega_s = 1.0 / (float(real_sample_count) * pdf);
            float omega_p = 4.0 * PI / (6.0 * width_sqr);
            float mip_level = roughness == 0.0 ? 0.0 : max(0.5 * log2(omega_s / omega_p), 0.0);
            float bias = min(mip_level / 4.0, 1.5); // bias reduces artefacts
            vec3 incoming = textureLod(cubemap_in, to_light, mip_level + bias).rgb * n_dot_l;
            prefiltered += incoming;
            weight += n_dot_l;
        }
    }

    prefiltered *= (1.0 / weight);

    imageStore(prefiltered_spec_out, cube_coords, vec4(prefiltered, 1.0));
}
