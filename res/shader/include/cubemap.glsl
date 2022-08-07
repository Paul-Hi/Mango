#ifndef MANGO_CUBEMAP_GLSL
#define MANGO_CUBEMAP_GLSL

#include <bindings.glsl>

layout(binding = CUBEMAP_DATA_BUFFER_BINDING_POINT, std140) uniform cubemap_data
{
    mat4 model_matrix;
    float render_level;
};

#endif // MANGO_CUBEMAP_GLSL