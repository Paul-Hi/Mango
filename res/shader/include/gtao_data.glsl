#ifndef MANGO_GTAO_DATA_GLSL
#define MANGO_GTAO_DATA_GLSL

#include <bindings.glsl>

layout(binding = GTAO_DATA_BUFFER_BINDING_POINT, std140) uniform gtao_data
{
    float ao_radius;                    // world space radius
    float thin_occluder_compensation;   // thin occluder compensation
    int slices;                         // number of slices in hemisphere
    int direction_samples;              // direction samples (in both directions so these times two are the actual sample count)
    int depth_mip_count;                // number of depth hierarchy mips for clamping
};

#endif // MANGO_GTAO_DATA_GLSL