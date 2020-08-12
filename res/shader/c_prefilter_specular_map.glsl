#version 430 core

#define saturate(x) clamp(x, 0.0, 1.0)

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = PI * 2.0;
const float INV_PI = 1.0 / PI;

const float width_sqr = 1024.0 * 1024.0; // TODO Paul: Hardcoded -.-
const uint sample_count = 512;

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, location = 0) uniform samplerCube cubemap_in;
layout(binding = 1, rgba16f) uniform writeonly imageCube prefiltered_spec_out;

layout(location = 1) uniform vec2 out_size;
layout(location = 2) uniform float roughness;

float radical_inverse_VdC(in uint bits);
vec2 sample_hammersley(uint i, uint sample_count);
void importance_sample_ggx_direction(in vec2 u, in vec3 view, in vec3 normal, in float roughness, out float n_dot_l, out vec3 halfway, out vec3 to_light);
vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size);
float D_GGX_divided_by_pi(in float n_dot_h, in float roughness);

vec3 up;
vec3 tangent_x;
vec3 tangent_y;

void main()
{
    ivec3 cube_coords = ivec3(gl_GlobalInvocationID.xyz);
    if (cube_coords.x >= out_size.x || cube_coords.y >= out_size.y)
        return;
    vec3 pos = cube_to_world(cube_coords, out_size);
    vec3 normal = normalize(pos);
    // assume view direction always equal to outgoing direction
    vec3 view = normal;
    vec3 ref = normal;

    up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    tangent_x = normalize(cross(up, normal));
    tangent_y = normalize(cross(normal, tangent_x));

    if (roughness == 0.0) {
        vec4 color = textureLod(cubemap_in, ref, 0);
        imageStore(prefiltered_spec_out, cube_coords, color);
        return;
    }

    vec3 prefiltered = vec3(0.0);
    float weight = 0.0;
    uint real_sample_count = uint(1.0 + float(sample_count) * roughness);

    for(uint s = 0; s < real_sample_count; ++s)
    {
        vec2 eta  = sample_hammersley(s, real_sample_count);
        vec3 halfway;
        vec3 to_light;
        float n_dot_l;
        importance_sample_ggx_direction(eta, view, normal, roughness, n_dot_l, halfway, to_light);
        n_dot_l = saturate(n_dot_l);

        if(n_dot_l > 0.0)
        {
            float n_dot_h = saturate(dot(normal, halfway));
            float h_dot_v = saturate(dot(halfway, view));
            float pdf = max(D_GGX_divided_by_pi(n_dot_h, roughness) * n_dot_h / (4.0 * h_dot_v), 1e-5);
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

// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// http://alinloghin.com/articles/compute_ibl.html
// https://xlgames-inc.github.io/posts/improvedibl/
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
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

vec2 sample_hammersley(uint i, uint sample_count)
{
    return vec2(float(i) / float(sample_count), radical_inverse_VdC(i));
}


void importance_sample_ggx_direction(in vec2 u, in vec3 view, in vec3 normal, in float roughness, out float n_dot_l, out vec3 halfway, out vec3 to_light)
{
    float u1 = u.x;
    float u2 = u.y;

    float a_sqr = roughness * roughness;
    float a_q = a_sqr * a_sqr;

    float phi = u1 * TWO_PI;
    float cos_theta2 = (1.0 - u2) / (1.0 + (a_q - 1.0) * u2);
    float cos_theta = sqrt(cos_theta2);
    float sin_theta = sqrt(1.0 - cos_theta2);

    halfway = vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    halfway = normalize(tangent_x * halfway.x + tangent_y * halfway.y + normal * halfway.z);

    to_light = normalize(2.0 * dot(view, halfway) * halfway - view);
    n_dot_l = dot(normal, to_light);
}

vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size)
{
    vec2 tex_coord = vec2(cube_coord.xy + 0.5) / cubemap_size;
    tex_coord = tex_coord * 2.0 - 1.0;
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

float D_GGX_divided_by_pi(in float n_dot_h, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float a_q = a_sqr * a_sqr;
    float f = (n_dot_h * a_q - n_dot_h) * n_dot_h + 1.0;
    return a_q / (PI * f * f);
}
