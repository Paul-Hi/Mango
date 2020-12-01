#ifndef MANGO_COMMON_PBR_GLSL
#define MANGO_COMMON_PBR_GLSL

#include <common_constants_and_functions.glsl>

//
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// http://alinloghin.com/articles/compute_ibl.html
// https://xlgames-inc.github.io/posts/improvedibl/
//

float D_GGX(in float n_dot_h, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float f = (n_dot_h * a_sqr - n_dot_h) * n_dot_h + 1.0;
    // Gets divided by pi later on.
    return a_sqr / (f * f);
}

float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float GGXL = n_dot_v * sqrt((-n_dot_l * a_sqr + n_dot_l) * n_dot_l + a_sqr);
    float GGXV = n_dot_l * sqrt((-n_dot_v * a_sqr + n_dot_v) * n_dot_v + a_sqr);
    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

vec3 F_Schlick(in float dot, in vec3 f0, in float f90) // can be optimized
{
    return f0 + (vec3(f90) - f0) * pow(saturate(1.0 - dot), 5.0);
}

vec3 F_Schlick_roughness(in float dot, in vec3 f0, in float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(saturate(1.0 - dot), 5.0);
}

float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness) // normalized Frostbyte version
{
    float energy_bias = mix(0.0, 0.5, roughness);
    float energy_factor = mix(1.0, 1.0 / 1.51, roughness);
    float f90 = energy_bias + 2.0 * l_dot_h * l_dot_h * roughness;
    vec3 f0 = vec3(1.0);
    float light_scatter = F_Schlick(n_dot_l, f0, f90).x;
    float view_scatter = F_Schlick(n_dot_v, f0, f90).x;
    // Gets divided by pi later on.
    return energy_factor * light_scatter * view_scatter;
}

vec3 get_specular_dominant_direction(in vec3 normal, in vec3 reflection, in float roughness)
{
    float smoothness = saturate(1.0 - roughness);
    float lerp_f = smoothness * (sqrt(smoothness) + roughness);
    return mix(normal, reflection, lerp_f);
}

void importance_sample_cosinus_direction(in vec2 u, in vec3 normal, in vec3 tangent_x, in vec3 tangent_y, out vec3 to_light, out float n_dot_l, out float pdf)
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

void importance_sample_ggx_direction(in vec2 u, in vec3 view, in vec3 normal, in vec3 tangent_x, in vec3 tangent_y, in float roughness, out float n_dot_l, out vec3 halfway, out vec3 to_light)
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



#endif // MANGO_COMMON_PBR_GLSL
