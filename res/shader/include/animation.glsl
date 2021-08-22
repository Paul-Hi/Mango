#ifndef MANGO_ANIMATION_GLSL
#define MANGO_ANIMATION_GLSL

#include <bindings.glsl>

layout(std430, binding = ANIMATION_DATA_BUFFER_BINDING_POINT) buffer animation_data
{
        mat4 joint_matrices[]; // The joint matrices.
};

#endif // MANGO_ANIMATION_GLSL
