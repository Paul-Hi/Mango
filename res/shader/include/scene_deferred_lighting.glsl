#ifndef MANGO_SCENE_DEFERRED_LIGHTING_GLSL
#define MANGO_SCENE_DEFERRED_LIGHTING_GLSL

#include <bindings.glsl>
#include <common_constants_and_functions.glsl>

out vec4 frag_color;

in vec2 texcoord;

layout(binding = GBUFFER_TEXTURE_SAMPLER_TARGET0) uniform sampler2D sampler_gbuffer_c0; // base color rgba (rgba8) // texture "texture_gbuffer_c0"
layout(binding = GBUFFER_TEXTURE_SAMPLER_TARGET1) uniform sampler2D sampler_gbuffer_c1; // normal rgb, alpha unused (rgb10a2) // texture "texture_gbuffer_c1"
layout(binding = GBUFFER_TEXTURE_SAMPLER_TARGET2) uniform sampler2D sampler_gbuffer_c2; // emissive rgb, alpha unused (rgba8) // texture "texture_gbuffer_c2"
layout(binding = GBUFFER_TEXTURE_SAMPLER_TARGET3) uniform sampler2D sampler_gbuffer_c3; // occlusion r, roughness g, metallic b, alpha unutexture_sed (rgba8) // texture "texture_gbuffer_c3"
layout(binding = GBUFFER_TEXTURE_SAMPLER_DEPTH) uniform sampler2D sampler_gbuffer_depth; // depth (d32) // texture "texture_gbuffer_depth"

layout(binding = IBL_SAMPLER_IRRADIANCE_MAP) uniform samplerCube sampler_irradiance_map; // texture "texture_irradiance_map"
layout(binding = IBL_SAMPLER_RADIANCE_MAP) uniform samplerCube sampler_radiance_map; // texture "texture_radiance_map"
layout(binding = IBL_SAMPLER_LOOKUP) uniform sampler2D sampler_brdf_integration_lut; // texture "texture_brdf_integration_lut"

layout(binding = SAMPLER_SHADOW_SHADOW_MAP) uniform sampler2DArrayShadow sampler_shadow_shadow_map; // texture "texture_shadow_map_comp"
layout(binding = SAMPLER_SHADOW_MAP) uniform sampler2DArray sampler_shadow_map; // texture "texture_shadow_map"

#include <renderer.glsl>

#include <camera.glsl>

#include <light.glsl>

#include <shadow.glsl>

vec4 get_base_color()
{
    return texture(sampler_gbuffer_c0, texcoord);
}

vec3 get_emissive()
{
    return texture(sampler_gbuffer_c2, texcoord).rgb;
}

vec3 get_occlusion_roughness_metallic()
{
    vec3 o_r_m = texture(sampler_gbuffer_c3, texcoord).rgb;
    o_r_m.x = max(o_r_m.x, 0.089f);
    return o_r_m;
}

vec3 get_normal()
{
    return normalize(texture(sampler_gbuffer_c1, texcoord).rgb * 2.0 - 1.0);
}

float get_logarithmic_depth()
{
    return texture(sampler_gbuffer_depth, texcoord).r;
}

void draw_debug_views()
{
    float depth                = get_logarithmic_depth();
    vec3 position              = world_space_from_depth(depth, texcoord, inverse_view_projection);
    vec4 base_color            = get_base_color();
    vec3 normal                = get_normal();
    vec3 o_r_m                 = get_occlusion_roughness_metallic();
    float occlusion_factor     = o_r_m.r;
    float perceptual_roughness = o_r_m.g;
    float metallic             = o_r_m.b;
    vec3 emissive              = get_emissive();

    if(position_debug_view)
    {
        frag_color = vec4(position, 1.0);
        return;
    }
    if(normal_debug_view)
    {
        frag_color = vec4(normal, 1.0);
        return;
    }
    if(depth_debug_view)
    {
        float z_lin = linearize_depth(depth, camera_near, camera_far);
        frag_color = vec4(vec3(z_lin), 1.0);
        return;
    }
    if(base_color_debug_view)
    {
        frag_color = vec4(vec3(1.0 - metallic) * base_color.rgb, 1.0);
        return;
    }
    if(reflection_color_debug_view)
    {
        frag_color = vec4(vec3(metallic) * base_color.rgb, 1.0);
        return;
    }
    if(occlusion_debug_view)
    {
        frag_color = vec4(vec3(occlusion_factor), 1.0);
        return;
    }
    if(roughness_debug_view)
    {
        frag_color = vec4(vec3(perceptual_roughness), 1.0);
        return;
    }
    if(metallic_debug_view)
    {
        frag_color = vec4(vec3(metallic), 1.0);
        return;
    }
    if(emission_debug_view)
    {
        frag_color = vec4(vec3(emissive), 1.0);
        return;
    }
}

#endif // MANGO_SCENE_DEFERRED_LIGHTING_GLSL