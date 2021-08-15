#include <../include/scene_geometry.glsl>
#include <../include/lighting_functions.glsl>
#include <../include/shadow_functions.glsl>

void main()
{
    if(debug_view_enabled)
    {
        draw_debug_views();
        return;
    }

    vec4 base_color = get_base_color();
    vec3 normal = get_normal();
    vec3 view = normalize(camera_position.xyz - fs_in.position);
    float n_dot_v = clamp(dot(normal, view), 1e-5, 1.0 - 1e-5);
    vec3 o_r_m = get_occlusion_roughness_metallic();
    float occlusion = o_r_m.x;
    float perceptual_roughness = o_r_m.y;
    float metallic = o_r_m.z;
    float reflectance = 0.5; // TODO Paul: Make tweakable.
    vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + base_color.rgb * metallic;


    // skylight
    vec3 skylight_contribution = calculate_skylight(base_color.rgb, normal, view, n_dot_v, perceptual_roughness, metallic, f0, occlusion);

    // lights
    vec3 directional_contribution = calculate_directional_light(base_color.rgb, normal, view, n_dot_v, perceptual_roughness, metallic, f0, occlusion);

    float shadow = 1.0;
    vec3 cascade_color = vec3(1.0);

    // shadows (directional)
    if(shadow_step_enabled && directional_valid && directional_cast_shadows)
    {
        shadow = directional_shadow(fs_in.position, normal);
        if(show_cascades)
            cascade_color = get_shadow_cascade_debug_color(fs_in.position);
    }

    vec3 lighting = vec3(0.0);
    lighting += skylight_contribution;
    lighting += directional_contribution * shadow;
    lighting += get_emissive();

    lighting *= cascade_color;

    frag_color = vec4(lighting * base_color.a, base_color.a); // Premultiplied alpha?
}
