#ifndef MANGO_LIGHT_GLSL
#define MANGO_LIGHT_GLSL

#include <bindings.glsl>

layout(binding = LIGHT_DATA_BUFFER_BINDING_POINT, std140) uniform light_data
{
    vec4  directional_direction; // this is a vec3, but there are annoying bugs with some drivers.
    vec4  directional_color; // this is a vec3, but there are annoying bugs with some drivers.
    float directional_intensity;
    bool  directional_cast_shadows;
    bool  directional_valid;

    float skylight_intensity;
    bool  skylight_valid;
};

#endif // MANGO_LIGHT_GLSL