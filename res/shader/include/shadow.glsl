#ifndef MANGO_SHADOW_GLSL
#define MANGO_SHADOW_GLSL

#include <bindings.glsl>

#define MAX_SHADOW_CASCADES 4

layout(binding = SHADOW_DATA_BUFFER_BINDING_POINT, std140) uniform shadow_data
{
    mat4  shadow_view_projection_matrices[MAX_SHADOW_CASCADES];
    float split_depth[MAX_SHADOW_CASCADES];
    vec4  shadow_far_planes;
    int   shadow_resolution;
    int   shadow_cascade_count;
    float shadow_cascade_interpolation_range;
    int   shadow_sample_count;
    float shadow_slope_bias;
    float shadow_normal_bias;
    int   shadow_filter_mode;
    float shadow_width;
    float shadow_light_size;
    int   cascade;
};

#endif // MANGO_SHADOW_GLSL