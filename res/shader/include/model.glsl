#ifndef MANGO_MODEL_GLSL
#define MANGO_MODEL_GLSL

#include <bindings.glsl>

layout(binding = MODEL_DATA_BUFFER_BINDING_POINT, std140) uniform model_data
{
    mat4  model_matrix;  // The model matrix.
    mat3  normal_matrix; // The normal matrix.
    bool  has_normals;   // Specifies if the mesh has normals as a vertex attribute.
    bool  has_tangents;  // Specifies if the mesh has tangents as a vertex attribute.
    float padding0;      // Padding.
    float padding1;      // Padding.
};

#endif // MANGO_MODEL_GLSL