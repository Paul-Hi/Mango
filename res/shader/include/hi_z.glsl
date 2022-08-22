#ifndef MANGO_HI_Z_GLSL
#define MANGO_HI_Z_GLSL

#include <bindings.glsl>

layout(binding = HI_Z_DATA_BUFFER_BINDING_POINT, std140) uniform hi_z_data
{
    vec4 params; // x,y are the size of the output and z,w are 1 / (x, y) in first pass (afterwards the size of the input)
    int pass;
};

#endif // MANGO_HI_Z_GLSL