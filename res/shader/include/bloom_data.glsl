#ifndef MANGO_BLOOM_DATA_GLSL
#define MANGO_BLOOM_DATA_GLSL

#include <bindings.glsl>

layout(binding = BLOOM_DATA_BUFFER_BINDING_POINT, std140) uniform bloom_data
{
    int filter_radius;                // filter radius in texel space
    float power;                        // power of bloom
    int current_mip;                    // The current mip level to read from
};

#endif // MANGO_BLOOM_DATA_GLSL