#version 430 core

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = PI * 2.0;
const float INV_PI = 1.0 / PI;

const uint sample_count = 512; // sufficient because of the mipmap optimization -> we would need more without!
const float inverse_sample_count = 1.0 / float(sample_count);
const float width_sqr = 1024.0 * 1024.0;

layout(local_size_x = 32, local_size_y = 32) in;

layout(binding = 0, location = 0) uniform samplerCube cubemap_in;
layout(binding = 1, rgba16f) uniform writeonly imageCube irradiance_map_out;

layout(location = 1) uniform vec2 out_size;

float radical_inverse_VdC(in uint bits);
vec2 sample_hammersley(uint i);
void importance_sample_cosinus_direction(in vec2 u, in vec3 normal, out vec3 to_light, out float n_dot_l, out float pdf);
vec3 cube_to_world(in ivec3 cube_coord, in vec2 cubemap_size);

vec3 up;
vec3 tangent_x;
vec3 tangent_y;

void main()
{
    ivec3 cube_coords = ivec3(gl_GlobalInvocationID.xyz);
    vec3 pos = cube_to_world(cube_coords, out_size);
    vec3 normal = normalize(pos);

    up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    tangent_x = normalize(cross(up, normal));
    tangent_y = normalize(cross(normal, tangent_x));


    vec3 irradiance = vec3(0.0);

    for(uint s = 0; s < sample_count; ++s)
    {
        vec2 eta  = sample_hammersley(s);
        vec3 to_light;
        float n_dot_l;
        float pdf;
        importance_sample_cosinus_direction(eta, normal, to_light, n_dot_l, pdf);

        if(n_dot_l > 0.0)
        {
            // cosine weighted hemisphere pdf is cos(theta) / PI
            // assuming average solid angle in the input cube map
            // Omega_p = (4*PI) / (fCubeMapDim * fCubeMapDim * 6)
            // Omega_s = PI / (fSampleCount * cos(theta))
            float o = (6.0 * width_sqr) / (4.0 * float(sample_count) * n_dot_l);
            float mip_level = max(0.5 * log2(o), 0.0);
            float bias = min(mip_level / 4.0, 1.5); // bias reduces artefacts
            irradiance += textureLod(cubemap_in, to_light, mip_level + bias).rgb * n_dot_l;
        }
    }

    irradiance *= PI * inverse_sample_count;

    imageStore(irradiance_map_out, cube_coords, vec4(irradiance, 1.0));
}

// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// http://alinloghin.com/articles/compute_ibl.html
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

vec2 sample_hammersley(uint i)
{
    return vec2(float(i) * inverse_sample_count, radical_inverse_VdC(i));
}

void importance_sample_cosinus_direction(in vec2 u, in vec3 normal, out vec3 to_light, out float n_dot_l, out float pdf)
{
    float u1 = u.x;
    float u2 = u.y;

    float r = sqrt(u1);
    float phi = u2 * TWO_PI;

    to_light = vec3(r * cos(phi), r * sin(phi), sqrt(max(0.0f, 1.0f - u1)));
    to_light = normalize(tangent_x * to_light.x + tangent_y * to_light.y + normal * to_light.z);

    n_dot_l = dot(normal, to_light);
    pdf = n_dot_l * INV_PI;
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
