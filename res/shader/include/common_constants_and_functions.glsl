#ifndef MANGO_COMMON_CONSTANTS_AND_FUNCTIONS_GLSL
#define MANGO_COMMON_CONSTANTS_AND_FUNCTIONS_GLSL

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = PI * 2.0;
const float INV_PI = 1.0 / PI;

const float DFG_TEXTURE_SIZE = 256.0;
const float MAX_REFLECTION_LOD = 10.0;

#define saturate(x) clamp(x, 0.0, 1.0)
#define epsilon 0.005

float pow5 (in float v)
{
    return v * v * v * v * v;
}

float linearize_depth(in float d, in float near, in float far)
{
    float depth = d * 2.0 - 1.0;
    return 2.0 * near / (far + near - depth * (far - near));
}

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection)
{
    float z = depth * 2.0 - 1.0;

    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 direct = inverse_view_projection * clip;
    return direct.xyz / direct.w;
}

float interleaved_gradient_noise(in vec2 pos)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(pos, magic.xy)));
}

vec2 vogel_disc_sample(in int sample_idx, in int sample_count, in float rotation)
{
    float golden_angle = 2.399963;

    float r = sqrt(sample_idx + 0.5) / sqrt(sample_count);
    float theta = sample_idx * golden_angle + rotation;

    float s = sin(theta);
    float c = cos(theta);

    return vec2(c, s) * r;
}

#ifndef COMPUTE

bool alpha_dither(in vec2 screen_pos, in float alpha) {

    const float thresholds[16] =
    {
        1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
    };

    int x = int(mod(screen_pos.x, 4.0));
    int y = int(mod(screen_pos.y, 4.0));

    float limit = thresholds[x + (y * 4)];

    if(alpha < limit)
        return true;

    return false;
}

#endif // COMPUTE

vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size)
{
    vec2 tex_coord = vec2(cube_coord.xy + 0.5) / cubemap_size;
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

vec2 intersect_ray_sphere(in vec3 origin, in vec3 dir, in float sphere_radius) {
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, origin);
    float c = dot(origin, origin) - (sphere_radius * sphere_radius);
    float d = (b * b) - 4.0 * a * c;

    if (d < 0.0)
        return vec2(1e5, -1e5);
    return vec2((-b - sqrt(d)) / (2.0 * a), (-b + sqrt(d)) / (2.0 * a));
}

float luma(in vec4 color)
{
    return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

float luma(in vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec4 srgb_to_linear(in vec4 srgb)
{
    return vec4(pow(srgb.rgb, vec3(2.2)), srgb.a);
}

vec4 linear_to_srgb(in vec4 linear)
{
    return vec4(pow(linear.rgb, vec3(1.0 / 2.2)), linear.a);
}

// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radical_inverse_VdC(in uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 sample_hammersley(in uint i, in float inverse_sample_count)
{
    return vec2(float(i) * inverse_sample_count, radical_inverse_VdC(i));
}


#endif // MANGO_COMMON_CONSTANTS_AND_FUNCTIONS_GLSL
