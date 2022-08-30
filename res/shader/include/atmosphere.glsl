#ifndef MANGO_ATMOSPHERE_GLSL
#define MANGO_ATMOSPHERE_GLSL

#include <bindings.glsl>

layout(binding = ATMOSPHERE_DATA_BUFFER_BINDING_POINT, std140) uniform atmosphere_data
{
    vec3 sun_dir;
    float sun_intensity;
    vec3 rayleigh_scattering_coefficients;
    float ground_radius;
    vec3 ray_origin;
    float mie_scattering_coefficient;
    vec2 density_multiplier;
    float atmosphere_radius;
    float mie_preferred_scattering_dir;
    vec2 out_size;
    int scatter_points;
    int scatter_points_second_ray;
    bool draw_sun_disc;
};

#endif // MANGO_ATMOSPHERE_GLSL
