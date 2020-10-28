#version 430 core

out vec4 frag_color;

in vec3 shared_texcoord;

layout (location = 0, binding = 0) uniform samplerCube skybox;

// Uniform Buffer Lighting Pass.
layout(binding = 3, std140) uniform lighting_pass_data
{
    mat4 inverse_view_projection;
    mat4 view;
    vec4 camera_position; // this is a vec3, but there are annoying bugs with some drivers.
    vec4 camera_params; // near, far, (zw) unused

    vec4  directional_direction; // this is a vec3, but there are annoying bugs with some drivers.
    vec4  directional_color; // this is a vec3, but there are annoying bugs with some drivers.
    float directional_intensity;
    bool  cast_shadows;

    float ambient_intensity;

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

// Uniform Buffer IBL.
layout(binding = 5, std140) uniform ibl_data
{
    mat3 current_rotation_scale;
    float render_level;
};

void main()
{
    vec3 color = (textureLod(skybox, shared_texcoord, render_level) * ambient_intensity).rgb;

    frag_color = vec4(color, 1.0);
}
