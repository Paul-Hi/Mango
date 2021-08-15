
#include <include/common_constants_and_functions.glsl>

#define F16_MAX 65500.0
#define F16_MIN 0.000655

const vec2 inv_atan = vec2(0.15915, 0.31831);

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0) uniform sampler2D sampler_hdr_in; // texture "texture_hdr_in"
layout(binding = 1, rgba16f) uniform writeonly imageCube cubemap_out;

layout(binding = 3) uniform ibl_generation_data
{
    vec2 out_size;
    vec2 data;
};

vec2 cube_to_equi(in vec3 v);

void main()
{
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);
    ivec3 coords = ivec3(gl_GlobalInvocationID);

    vec3 pos = cube_to_world(coords, out_size);

    vec2 uv = cube_to_equi(normalize(pos));
    uv.y = 1.0 - uv.y;

    pixel.rgb = texture(sampler_hdr_in, uv).rgb;

    pixel.rgb = clamp(pixel.rgb, vec3(F16_MIN), vec3(F16_MAX));

    imageStore(cubemap_out, coords, pixel);
}

vec2 cube_to_equi(in vec3 v)
{
    vec2 theta_pi = vec2(atan(v.z, v.x), asin(v.y));
    theta_pi *= inv_atan;
    theta_pi += 0.5;
    return theta_pi;
}
