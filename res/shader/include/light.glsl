#ifndef MANGO_LIGHT_GLSL
#define MANGO_LIGHT_GLSL

#include <bindings.glsl>

layout(binding = LIGHT_DATA_BUFFER_BINDING_POINT, std140) uniform light_data
{
    vec3  directional_light_direction;
    vec3  directional_light_color;
    float directional_light_intensity;
    bool  directional_light_cast_shadows;
    bool  directional_light_valid;

    float skylight_intensity;
    bool  skylight_valid;
};

#endif // MANGO_LIGHT_GLSL