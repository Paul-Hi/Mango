#include <../include/common_constants_and_functions.glsl>
#include <../include/common_state.glsl>
#include <../include/common_lighting.glsl>
#include <../include/common_shadow.glsl>

void debug_views();

void main()
{
    populate_datapool();

    if(is_debug_view_enabled())
    {
        draw_debug_views();
        return;
    }

    bool shadow_maps_debug = texcoord.y < 0.25 && should_draw_shadow_maps() && should_directional_light_cast_shadows();
    if(shadow_maps_debug)
    {
        draw_shadow_maps_debug(texcoord);
        return;
    }

    // skylight
    vec3 skylight_contribution = calculate_skylight();

    // lights
    vec3 directional_contribution = calculate_directional_light();

    float shadow = 1.0;
    vec3 cascade_color = vec3(1.0);

    // shadows (directional)
    if(is_shadow_step_enabled() && is_directional_light_valid() && should_directional_light_cast_shadows())
    {
        shadow = directional_shadow();
        if(should_show_cascades())
            cascade_color = get_shadow_cascade_debug_color();
    }

    vec3 lighting = vec3(0.0);
    lighting += skylight_contribution;
    lighting += directional_contribution * shadow;
    lighting += get_emissive() * 50000.0; // TODO Paul: Remove hardcoded intensity for all emissive values -.-

    lighting *= cascade_color;

    frag_color = vec4(lighting * base_color.a, base_color.a); // Premultiplied alpha?
}
