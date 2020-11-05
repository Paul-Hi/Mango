#version 430 core

#define saturate(x) clamp(x, 0.0, 1.0)

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = PI * 2.0;
const float INV_PI = 1.0 / PI;

const uint sample_count = 512;
float inverse_sample_count = 1.0 / float(sample_count);

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, rgba16f) uniform writeonly image2D integration_lut_out;

layout(location = 0) uniform vec2 out_size;

float inv_tex_size = 1.0 / out_size.x;

float radical_inverse_VdC(in uint bits);
vec2 sample_hammersley(uint i);
void importance_sample_ggx_direction(in vec2 u, in vec3 view, in vec3 normal, in float roughness, out float n_dot_l, out vec3 halfway, out vec3 to_light);
float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness);

vec3 up;
vec3 tangent_x;
vec3 tangent_y;

void main()
{
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    if (coords.x >= out_size.x || coords.y >= out_size.y)
        return;
    float n_dot_v = float(coords.x) * inv_tex_size;
    float roughness = float(coords.y) * inv_tex_size;

    n_dot_v = clamp(n_dot_v, 1e-5, 1.0 - 1e-5);

    vec3 view = vec3(sqrt(1.0 - n_dot_v * n_dot_v), 0.0, n_dot_v);
    vec3 normal = vec3(0.0, 0.0, 1.0);

    up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    tangent_x = normalize(cross(up, normal));
    tangent_y = normalize(cross(normal, tangent_x));

    vec3 integration_lu = vec3(0.0);

    for(uint s = 0; s < sample_count; ++s)
    {
        vec2 eta = sample_hammersley(s);
        vec3 halfway;
        vec3 to_light;
        float n_dot_l;
        importance_sample_ggx_direction(eta, view, normal, roughness, n_dot_l, halfway, to_light);
        n_dot_l = saturate(n_dot_l);
        float G = V_SmithGGXCorrelated(n_dot_v, n_dot_l, roughness);

        // specular preintegration with multiscattering
        if(n_dot_l > 0.0 && G > 0.0)
        {
            float v_dot_h = saturate(dot(view, halfway));
            float n_dot_h = saturate(halfway.z);
            float G_vis = G * n_dot_l * (v_dot_h / n_dot_h );
            float Fc = pow(1.0 - v_dot_h, 5.0);
            // https://google.github.io/filament/Filament.html#listing_energycompensationimpl
            // Assuming f90 = 1
            integration_lu.x += Fc * G_vis;
            integration_lu.y += G_vis;
        }
    }

    integration_lu *= inverse_sample_count;

    imageStore(integration_lut_out, coords, vec4(integration_lu, 1.0));
}

// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// http://alinloghin.com/articles/compute_ibl.html
// https://xlgames-inc.github.io/posts/improvedibl/
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf

float radical_inverse_VdC(in uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 sample_hammersley(uint i)
{
    return vec2(float(i) * inverse_sample_count, radical_inverse_VdC(i));
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
    n_dot_l = to_light.z;
}

float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float a_q = a_sqr * a_sqr;
    float GGXL = n_dot_v * sqrt((-n_dot_l * a_q + n_dot_l) * n_dot_l + a_q);
    float GGXV = n_dot_l * sqrt((-n_dot_v * a_q + n_dot_v) * n_dot_v + a_q);
    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}
