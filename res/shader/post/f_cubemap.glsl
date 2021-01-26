#include <../include/common_constants_and_functions.glsl>

out vec4 frag_color;

in vec3 shared_texcoord;

layout (location = 0) uniform samplerCube skybox;

// Uniform Buffer Lighting Pass.
layout(binding = 1, std140) uniform lighting_pass_data
{
    mat4 inverse_view_projection;
    mat4 view;
    vec4 camera_position; // this is a vec3, but there are annoying bugs with some drivers.
    vec4 camera_params; // near, far, (zw) unused

    bool debug_view_enabled;
    bool debug_views_position;
    bool debug_views_normal;
    bool debug_views_depth;
    bool debug_views_base_color;
    bool debug_views_reflection_color;
    bool debug_views_emission;
    bool debug_views_occlusion;
    bool debug_views_roughness;
    bool debug_views_metallic;
    bool show_cascades;
    bool draw_shadow_maps;
};

layout(binding = 4, std140) uniform light_data
{
    vec4  directional_direction; // this is a vec3, but there are annoying bugs with some drivers.
    vec4  directional_color; // this is a vec3, but there are annoying bugs with some drivers.
    float directional_intensity;
    bool  directional_cast_shadows;
    bool  directional_valid;

    float skylight_intensity;
    bool  skylight_valid;
};

// Uniform Buffer Cubemap.
layout(binding = 5, std140) uniform cubemap_data
{
    mat4 model_matrix;
    float render_level;
};

// Uniform Buffer Atmosphere Compute.
layout(binding = 6, std140) uniform atmosphere_ub_data
{
    vec4 sun_dir; // vec3 -> vec4
    vec4 rayleigh_scattering_coefficients; // vec3 -> vec4
    vec4 ray_origin; // vec3 -> vec4
    vec2 density_multiplier;
    float sun_intensity;
    float mie_scattering_coefficient;
    float ground_radius;
    float atmosphere_radius;
    float mie_preferred_scattering_dir;
    int scatter_points;
    int scatter_points_second_ray;
};

void main()
{
    vec3 color;
    if(skylight_valid && skylight_intensity > 1e-5)
    {
        color = (textureLod(skybox, shared_texcoord, render_level) * skylight_intensity).rgb;

        // vec3 ray_dir = normalize(shared_texcoord);
        // vec2 ground_intersect = intersect_ray_sphere(ray_origin.xyz, ray_dir, ground_radius);

        // // add a sun disc
        // vec3 sun = vec3(pow(smoothstep(0.997, 1.0, dot(ray_dir, sun_dir.xyz)), 16.0));
        // sun *= saturate(pow(smoothstep(0.0, ground_radius, abs(ground_intersect.x) - ground_intersect.y), 2.0));

        // color += directional_color.rgb * directional_intensity * skylight_intensity * sun * 0.007;
    }
    else
        color = vec3(30000.0); // TODO Paul: Hardcoded -.-
    frag_color = vec4(color, 1.0);
}
