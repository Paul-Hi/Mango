#ifndef MANGO_COMMON_LIGHTING_GLSL
#define MANGO_COMMON_LIGHTING_GLSL

#include <common_constants_and_functions.glsl>
#include <common_getter_setter.glsl>
#include <common_pbr.glsl>

vec3 calculate_skylight()
{
    float light_intensity = get_skylight_intensity();
    if(!is_skylight_valid() || light_intensity < 1e-5)
        return vec3(300.0); // TODO Paul: Hardcoded -.-

    float n_dot_v              = get_n_dot_v();
    vec3 normal                = get_normal();
    float perceptual_roughness = get_perceptual_roughness();
    vec3 f0                    = get_f0();

    n_dot_v = clamp(n_dot_v , 0.5 / DFG_TEXTURE_SIZE, 1.0 - 0.5 / DFG_TEXTURE_SIZE);
    vec3 dfg = textureLod(brdf_integration_lut, saturate(vec2(n_dot_v, perceptual_roughness)), 0.0).xyz;
    // irradiance
    vec3 irradiance  = texture(irradiance_map, normal).rgb;
    vec3 diffuse     = irradiance * get_real_albedo();
    vec3 diffuse_ibl = diffuse;

    // specular
    vec3 refl              = -normalize(reflect(get_view_direction(), normal));
    vec3 dominant_refl     = get_specular_dominant_direction(normal, refl, perceptual_roughness);
    float mip_index        = perceptual_roughness * MAX_REFLECTION_LOD;
    vec3 prefiltered_color = textureLod(prefiltered_specular, dominant_refl, mip_index).rgb;
    vec3 specular_ibl      = prefiltered_color * mix(dfg.xxx, dfg.yyy, f0);

    vec3 energy_compensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
    specular_ibl *= energy_compensation;

    return (diffuse_ibl * get_occlusion() + specular_ibl) * light_intensity;
}

vec3 calculate_directional_light()
{
    float light_intensity = get_directional_light_intensity();
    if(!is_directional_light_valid() || light_intensity < 1e-5)
        return vec3(0.0);

    float n_dot_v              = get_n_dot_v();
    vec3 normal                = get_normal();
    float perceptual_roughness = get_perceptual_roughness();

    vec3 light_dir        = normalize(get_directional_light_direction());
    vec3 light_col        = get_directional_light_color();
    float roughness       = (perceptual_roughness * perceptual_roughness);

    // adjust roughness to approximate small disk
    float lightRoughness = 0.1 * 696340.0 / 14960000.0; // sun radius / sun distance * 0.1 -> some approximation.
    float specular_roughness = saturate(lightRoughness + roughness);

    vec3 lighting = vec3(0.0);

    vec3 halfway  = normalize(light_dir + get_view_direction());
    float n_dot_l = saturate(dot(normal, light_dir));
    float n_dot_h = saturate(dot(normal, halfway));
    float l_dot_h = saturate(dot(light_dir, halfway));

    float D = D_GGX(n_dot_h, specular_roughness);
    vec3 F  = F_Schlick(l_dot_h, get_f0(), 1.0);
    float V = V_SmithGGXCorrelated(n_dot_v, n_dot_l, specular_roughness);

    // Fr energy compensation
    vec3 Fr = D * V * F * (1.0 / PI);

    vec3 Fd = get_real_albedo() * Fd_BurleyRenormalized(n_dot_v, n_dot_l, l_dot_h, roughness) * (1.0 / PI);

    vec3 diffuse = n_dot_l * Fd;
    vec3 specular = n_dot_l * Fr;

    lighting += (diffuse * get_occlusion() + specular) * light_col * light_intensity;

    return lighting;
}

#endif // MANGO_COMMON_LIGHTING_GLSL
