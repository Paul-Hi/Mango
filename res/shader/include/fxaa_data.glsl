#ifndef MANGO_FXAA_DATA_GLSL
#define MANGO_FXAA_DATA_GLSL

#include <bindings.glsl>

layout(binding = FXAA_DATA_BUFFER_BINDING_POINT, std140) uniform fxaa_data
{
    vec2 inverse_screen_size;
    float subpixel_filter;
};

#endif // MANGO_FXAA_DATA_GLSL