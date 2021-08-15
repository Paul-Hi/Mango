#ifndef MANGO_CAMERA_GLSL
#define MANGO_CAMERA_GLSL

#include <bindings.glsl>

layout(binding = CAMERA_DATA_BUFFER_BINDING_POINT, std140) uniform camera_data
{
    mat4  view_matrix;             // The view matrix.
    mat4  projection_matrix;       // The projection matrix.
    mat4  view_projection_matrix;  // The view projection matrix.
    mat4  inverse_view_projection; // Inverse camera view projection matrix.
    vec4  camera_position;         // Camera position. This is a vec3, but there are annoying bugs with some drivers.
    float camera_near;             // Camera near plane depth value.
    float camera_far;              // Camera far plane depth value.
    float camera_exposure;         // The exposure value of the camera.
};

#endif // MANGO_CAMERA_GLSL