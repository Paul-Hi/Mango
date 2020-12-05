#include <../include/common_constants_and_functions.glsl>

out vec4 frag_color;

in vec2 texcoord;

layout(location = 0) uniform sampler2D hdr_input;

// Uniform Buffer Renderer.
layout(binding = 0, std140) uniform renderer_data
{
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    float camera_exposure;
    bool shadow_step_enabled;
};

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

vec4 tonemap_with_gamma_correction(in vec4 color);

void main()
{
    bool no_correction = debug_view_enabled || (draw_shadow_maps && texcoord.y < 0.25);
    if (no_correction) // TODO Paul: This is weird.
    {
        frag_color = texture(hdr_input, texcoord);
        return;
    }

    frag_color = tonemap_with_gamma_correction(texture(hdr_input, texcoord));
}

vec3 uncharted2_tonemap(in vec3 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color * (A * color + C * B) + D * E)/(color * (A * color + B) + D * F)) - E / F;
}

vec4 tonemap_with_gamma_correction(in vec4 color)
{
    // tonemapping // TODO Paul: There is room for improvement. Gamma parameter?
    const float W = 11.2;
    vec3 outcol = uncharted2_tonemap(color.rgb * camera_exposure * 4.0);
    outcol /= uncharted2_tonemap(vec3(W));
    return linear_to_srgb(vec4(outcol, color.a)); // gamma correction.
}
