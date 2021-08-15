#ifndef MANGO_LIGHTING_FUNCTIONS_GLSL
#define MANGO_LIGHTING_FUNCTIONS_GLSL

#include <common_constants_and_functions.glsl>
#include <pbr_functions.glsl>

vec3 calculate_skylight(in vec3 base_color, in vec3 normal, in vec3 view, in float n_dot_v, in float perceptual_roughness, in float metallic, in vec3 f0, in float occlusion)
{
    vec3 albedo      = base_color * (1.0 - metallic);
    if(!skylight_valid || skylight_intensity < 1e-5)
        return vec3(300.0) * albedo; // TODO Paul: Hardcoded -.-

    n_dot_v = max(n_dot_v , 0.5 / DFG_TEXTURE_SIZE);
    vec3 dfg = textureLod(sampler_brdf_integration_lut, saturate(vec2(n_dot_v, perceptual_roughness)), 0.0).xyz;
    // irradiance
    vec3 irradiance  = texture(sampler_irradiance_map, normal).rgb;
    vec3 diffuse     = irradiance * albedo;
    vec3 diffuse_ibl = diffuse;

    // specular
    vec3 refl              = -normalize(reflect(view, normal));
    float alpha            = perceptual_roughness* perceptual_roughness;
    vec3 dominant_refl     = get_specular_dominant_direction(normal, refl, alpha);
    float mip_index        = perceptual_roughness * MAX_REFLECTION_LOD;
    vec3 prefiltered_color = textureLod(sampler_radiance_map, dominant_refl, mip_index).rgb;

    // test optimization for high roughness - low mip problem. This is not perfectly accurate, but a lot nicer to the eye!
    float metallic_high_res_luma = texture(sampler_irradiance_map, normal).a;
    float inv_irradiance_luma = 1.0 / luma(irradiance);
    float d = smoothstep(0.5, 1.0, perceptual_roughness);
    prefiltered_color = mix(prefiltered_color, vec3(metallic_high_res_luma) * inv_irradiance_luma * irradiance, d);

    vec3 specular_ibl = prefiltered_color * mix(dfg.xxx, dfg.yyy, f0);

    vec3 energy_compensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
    specular_ibl *= energy_compensation;

    return (diffuse_ibl * occlusion + specular_ibl) * skylight_intensity;
}

vec3 calculate_directional_light(in vec3 base_color, in vec3 normal, in vec3 view, in float n_dot_v, in float perceptual_roughness, in float metallic, in vec3 f0, in float occlusion)
{
    if(!directional_valid || directional_intensity < 1e-5)
        return vec3(0.0);

    vec3 light_dir        = normalize(directional_direction.xyz);
    float alpha           = (perceptual_roughness * perceptual_roughness);

    // adjust roughness to approximate small disk
    float light_alpha = 0.1 * 696340.0 / 14960000.0; // sun radius / sun distance * 0.1 -> some approximation.
    float specular_alpha = saturate(light_alpha + alpha);

    vec3 lighting = vec3(0.0);

    vec3 halfway  = normalize(light_dir + view);
    float n_dot_l = saturate(dot(normal, light_dir));
    float n_dot_h = saturate(dot(normal, halfway));
    float l_dot_h = saturate(dot(light_dir, halfway));

    float D = D_GGX(n_dot_h, specular_alpha);
    vec3 F  = F_Schlick(l_dot_h, f0, 1.0);
    float V = V_SmithGGXCorrelated(n_dot_v, n_dot_l, specular_alpha);

    // Fr energy compensation
    vec3 Fr = D * V * F * INV_PI;

    vec3 albedo = base_color * (1.0 - metallic);

    vec3 Fd = albedo * Fd_BurleyRenormalized(n_dot_v, n_dot_l, l_dot_h, alpha) * INV_PI;

    vec3 diffuse = n_dot_l * Fd;
    vec3 specular = n_dot_l * Fr;

    lighting += (diffuse * occlusion + specular) * directional_color.rgb * directional_intensity;

    return lighting;
}

#endif // MANGO_LIGHTING_FUNCTIONS_GLSL
