#ifndef MANGO_LUMINANCE_GLSL
#define MANGO_LUMINANCE_GLSL

#include <bindings.glsl>

layout(binding = LUMINANCE_DATA_BUFFER_BINDING_POINT, std430) buffer luminance_data
{
    uint histogram[256];
    vec4 params; // min_log_luminance (x), inverse_log_luminance_range (y), time coefficient (z), pixel_count (w)
    float luminance;
};

#endif // MANGO_LUMINANCE_GLSL