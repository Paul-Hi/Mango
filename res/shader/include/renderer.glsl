#ifndef MANGO_RENDERER_GLSL
#define MANGO_RENDERER_GLSL

#include <bindings.glsl>

layout(binding = RENDERER_DATA_BUFFER_BINDING_POINT, std140) uniform renderer_data
{
        bool shadow_step_enabled;         // True, if the shadow map step is enabled and shadows can be calculated.
        bool debug_view_enabled;          // True, if any debug view is enabled. Used to prevent too much branching.
        bool position_debug_view;         // Show the position data.
        bool normal_debug_view;           // Show the normal data.
        bool depth_debug_view;            // Show the depth data.
        bool base_color_debug_view;       // Show the base color.
        bool reflection_color_debug_view; // Show the reflection color.
        bool emission_debug_view;         // Show the emission value.
        bool occlusion_debug_view;        // Show the occlusion value.
        bool roughness_debug_view;        // Show the roughness value.
        bool metallic_debug_view;         // Show the metallic value.
        bool show_cascades;               // Show the shadow cascades.
};

#endif // MANGO_RENDERER_GLSL