#version 430 core

#define F16_MAX 65500.0
#define F16_MIN 0.000655

const vec2 inv_atan = vec2(0.15915, 0.31831);

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, location = 0) uniform sampler2D hdr_in;
layout(binding = 1, rgba16f) uniform writeonly imageCube cubemap_out;

layout(location = 1) uniform vec2 out_size;

vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size);
vec2 cube_to_equi(in vec3 v);

void main()
{
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);

    ivec3 coords = ivec3(gl_GlobalInvocationID);
    vec3 pos = cube_to_world(coords, out_size);

    vec2 uv = cube_to_equi(normalize(pos));
    uv.y = 1.0 - uv.y;

    pixel.rgb = texture(hdr_in, uv).rgb;
    pixel.rgb = clamp(pixel.rgb, vec3(F16_MIN), vec3(F16_MAX));

    imageStore(cubemap_out, coords, pixel);
}

vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size)
{
    vec2 tex_coord = vec2(cube_coord.xy) / cubemap_size;
    tex_coord = tex_coord  * 2.0 - 1.0;
    switch(cube_coord.z)
    {
        case 0: return vec3(1.0, -tex_coord.yx);
        case 1: return vec3(-1.0, -tex_coord.y, tex_coord.x);
        case 2: return vec3(tex_coord.x, 1.0, tex_coord.y);
        case 3: return vec3(tex_coord.x, -1.0, -tex_coord.y);
        case 4: return vec3(tex_coord.x, -tex_coord.y, 1.0);
        case 5: return vec3(-tex_coord.xy, -1.0);
    }
    return vec3(0.0);
}

vec2 cube_to_equi(in vec3 v)
{
    vec2 theta_pi = vec2(atan(v.z, v.x), asin(v.y));
    theta_pi *= inv_atan;
    theta_pi += 0.5;
    return theta_pi;
}
