#ifndef MANGO_IBL_GEN_GLSL
#define MANGO_IBL_GEN_GLSL

#include <bindings.glsl>

layout(binding = IBL_GEN_DATA_BUFFER_BINDING_POINT, std140) uniform ibl_generation_data
{
    vec2 out_size;
    vec2 data;
};

#endif // MANGO_IBL_GEN_GLSL
