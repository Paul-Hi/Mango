#ifndef MANGO_BINDING_DATA
#define MANGO_BINDING_DATA

#include <bindings.glsl>

#ifdef BIND_RENDERER_DATA_BUFFER

layout(binding = RENDERER_DATA_BUFFER_BINDING_POINT, std140) uniform renderer_data
{
    bool shadow_step_enabled;         // True, if the shadow map step is enabled and shadows can be calculated.
    bool debug_view_enabled;          // True, if any debug view is enabled. Used to prevent too much branching.
    bool position_debug_view;         // Show the position data.
    bool normal_debug_view;           // Show the normal data.
    bool depth_debug_view;            // Show the depth data.
    bool base_color_debug_view;       // Show the base color.
    bool reflection_color_debug_view; // Show the reflection color.
    bool emission_debug_view;         // Show the emission value.
    bool occlusion_debug_view;        // Show the occlusion value.
    bool roughness_debug_view;        // Show the roughness value.
    bool metallic_debug_view;         // Show the metallic value.
    bool show_cascades;               // Show the shadow cascades.
};

#endif // BIND_RENDERER_DATA_BUFFER

#ifdef BIND_CAMERA_DATA_BUFFER

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

#endif // BIND_CAMERA_DATA_BUFFER

#ifdef BIND_INDIRECT_COMMANDS_BUFFER

struct draw_elements_indirect_command
{
    uint count;
    uint instance_count;
    uint first_index;
    uint base_vertex;
    uint base_instance;
};

layout(std430, binding = INDIRECT_COMMANDS_BUFFER_BINDING_POINT) buffer indirect_commands
{
    draw_elements_indirect_command indirect_draws[]; // List of draw_elements_indirect_command
};

#endif // BIND_INDIRECT_COMMANDS_BUFFER

#ifdef BIND_CULL_DATA_BUFFER

layout(binding = CULL_DATA_BUFFER_BINDING_POINT, std140) uniform cull_data
{
    vec4 cull_camera_frustum_planes[6];
    int cull_draws_offset;
    int cull_draw_count;
};

#endif // BIND_CULL_DATA_BUFFER

#ifdef BIND_AABB_DATA_BUFFER

struct aabb
{
    vec3 center;
    vec3 extents;
};

layout(std430, binding = AABB_DATA_BUFFER_BINDING_POINT) readonly buffer aabb_data
{
    aabb cull_aabbs[]; // List of aabb
};

#endif // BIND_AABB_DATA_BUFFER

#ifdef BIND_MODEL_DATA_BUFFER

struct per_model_data
{
    mat4  model_matrix;  // The model matrix.
    mat3  normal_matrix; // The normal matrix.
    bool  has_normals;   // Specifies if the mesh has normals as a vertex attribute.
    bool  has_tangents;  // Specifies if the mesh has tangents as a vertex attribute.
};

layout(std430, binding = MODEL_DATA_BUFFER_BINDING_POINT) readonly buffer model_data
{
    per_model_data model_data_array[]; // List of per_model_data
};

#endif // BIND_MODEL_DATA_BUFFER

#ifdef BIND_MATERIAL_DATA_BUFFER

struct per_material_data
{
    vec4  base_color;                 // The base color (rgba). Also used as reflection color for metallic surfaces.
    vec4  emissive_color;             //  The emissive color of the material if existent, else (0, 0, 0).
    float metallic;                   // The metallic value of the material.
    float roughness;                  // The roughness of the material.
    bool  base_color_texture;         // Specifies, if the the base color texture is enabled.
    bool  roughness_metallic_texture; // Specifies, if the component texture is enabled for the metallic value and the roughness value.
    bool  occlusion_texture;          // Specifies, if the component texture is enabled for the occlusion value.
    bool  packed_occlusion;           // Specifies, if the occlusion value is packed into the r channel of the roughness_metallic_texture.
    bool  normal_texture;             // Specifies, if the normal texture is enabled.
    bool  emissive_color_texture;     // Specifies, if the the emissive color texture is enabled.
    float emissive_intensity;         // Specifies the intensity multiplier for the emissive value.
    int   alpha_mode;                 // Specifies the alpha mode to render the material with.
    float alpha_cutoff;               // Specifies the alpha cutoff value to render the material with.
};

layout(std430, binding = MATERIAL_DATA_BUFFER_BINDING_POINT) readonly buffer material_data
{
    per_material_data material_data_array[]; // List of per_material_data
};

#endif // BIND_MATERIAL_DATA_BUFFER

#ifdef BIND_INSTANCE_DRAW_DATA_BUFFER

struct per_instance_data
{
    int model_index;
    int material_index;
};

layout(std430, binding = DRAW_INSTANCE_DATA_BUFFER_BINDING_POINT) readonly buffer instance_data
{
    per_instance_data instance_data_array[]; // List of per_instance_data
};

#endif // BIND_INSTANCE_DRAW_DATA_BUFFER

#ifdef BIND_LIGHT_DATA_BUFFER

layout(binding = LIGHT_DATA_BUFFER_BINDING_POINT, std140) uniform light_data
{
    vec4  directional_direction;    // The direction to the light.
    vec4  directional_color;        // The light color.
    float directional_intensity;    // The intensity of the directional light in lumen.
    bool  directional_cast_shadows; // True, if shadows can be casted.
    bool  directional_valid;        // True, if buffer is valid.

    float skylight_intensity; // The intensity of the skylight in cd/m^2.
    bool  skylight_valid;     // True, if buffer is valid. Does also guarantee that textures are bound.
};

#endif // BIND_LIGHT_DATA_BUFFER

#ifdef BIND_SHADOW_DATA_BUFFER

#define MAX_SHADOW_CASCADES 4

layout(binding = SHADOW_DATA_BUFFER_BINDING_POINT, std140) uniform shadow_data
{
    mat4  shadow_view_projection_matrices[MAX_SHADOW_CASCADES]; // The view projection matrices.
    float split_depth[MAX_SHADOW_CASCADES];                     // The calculated split depths.
    vec4  shadow_far_planes;                                    // The far planes of the shadow views.
    int   shadow_resolution;                                    // The shadow map resolution.
    int   shadow_cascade_count;                                 // The number of cascades.
    float shadow_cascade_interpolation_range;                   // The range to use for interpolating the cascades. Larger values mean smoother transition, but less quality and performance impact.
    int   shadow_sample_count;                                  // The sample count. Larger values can look more natural, but may cause artefacts and performance drops.
    float shadow_slope_bias;                                    // The slope bias.
    float shadow_normal_bias;                                   // The bias along the normal.
    int   shadow_filter_mode;                                   // shadow_filtering parameter.
    float shadow_width;                                         // Width of the PCF shadow.
    float shadow_light_size;                                    // Size of the light used for PCSS shadow.
    int   cascade;                                              // The currently rendered cascade. Only used in the rendering process, not required in th lookup while lighting is calculated.
};

#endif // BIND_SHADOW_DATA_BUFFER

#ifdef BIND_LUMINANCE_DATA_BUFFER

layout(binding = HDR_IMAGE_LUMINANCE_COMPUTE, rgba32f) uniform readonly image2D image_hdr_color;
layout(std430, binding = LUMINANCE_DATA_BUFFER_BINDING_POINT) buffer luminance_data
{
    uint histogram[256];
    vec4 params; // min_log_luminance (x), inverse_log_luminance_range (y), time coefficient (z), pixel_count (w)
    float luminance;
};

#endif // BIND_LUMINANCE_DATA_BUFFER

#ifdef BIND_IBL_GENERATION_DATA_BUFFER

layout(binding = IBL_GENERATION_DATA_BUFFER_BINDING_POINT) uniform ibl_generation_data
{
    vec2 out_size;
    vec2 data;
};

#endif // BIND_IBL_GENERATION_DATA_BUFFER

#ifdef BIND_IBL_BRDF_INTEGRATION_DATA

layout(binding = IBL_INTEGRATION_LUT, rgba16f) uniform writeonly image2D integration_lut_out;

#endif // BIND_IBL_BRDF_INTEGRATION_DATA

#ifdef BIND_IBL_IRRADIANCE_GENERATION_DATA

layout(binding = IBL_SAMPLER_CUBEMAP) uniform samplerCube sampler_cubemap_in; // texture "texture_cubemap_in"
layout(binding = IBL_IMAGE_CUBE_OUT, rgba16f) uniform writeonly imageCube irradiance_map_out;

#endif // BIND_IBL_IRRADIANCE_GENERATION_DATA

#ifdef BIND_IBL_SPECULAR_GENERATION_DATA

layout(binding = IBL_SAMPLER_CUBEMAP) uniform samplerCube sampler_cubemap_in; // texture "texture_cubemap_in"
layout(binding = IBL_IMAGE_CUBE_OUT, rgba16f) uniform writeonly imageCube prefiltered_spec_out;

#endif // BIND_IBL_SPECULAR_GENERATION_DATA



#endif // MANGO_BINDING_DATA