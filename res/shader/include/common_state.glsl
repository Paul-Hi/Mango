#ifndef MANGO_COMMON_STATE_GLSL
#define MANGO_COMMON_STATE_GLSL

#ifdef LIGHTING

struct
{
    vec4  base_color;
    float logarithmic_depth;
    vec3  world_space_position;
    vec3  normal;
    vec3  emissive;
    float occlusion;
    float perceptual_roughness;
    float metallic;

    vec3 view_direction;
    float n_dot_v;

    float reflectance;
    vec3 f0;
    vec3 real_albedo;
} shader_datapool;

vec4 get_base_color()
{
    return shader_datapool.base_color;
}

float get_logarithmic_depth()
{
    return shader_datapool.logarithmic_depth;
}

vec3 get_world_space_position()
{
    return shader_datapool.world_space_position;
}

vec3 get_normal()
{
    return shader_datapool.normal;
}

vec3 get_emissive()
{
    return shader_datapool.emissive;
}

float get_occlusion()
{
    return shader_datapool.occlusion;
}

float get_perceptual_roughness()
{
    return shader_datapool.perceptual_roughness;
}

float get_metallic()
{
    return shader_datapool.metallic;
}

float get_reflectance()
{
    return shader_datapool.reflectance;
}

vec3 get_f0()
{
    return shader_datapool.f0;
}

vec3 get_real_albedo()
{
    return shader_datapool.real_albedo;
}

out vec4 frag_color;

#ifdef DEFERRED
in vec2 texcoord;

layout(location = 0) uniform sampler2D gbuffer_c0; // base color rgba (rgba8)
layout(location = 1) uniform sampler2D gbuffer_c1; // normal rgb, alpha unused (rgb10a2)
layout(location = 2) uniform sampler2D gbuffer_c2; // emissive rgb, alpha unused (rgba8)
layout(location = 3) uniform sampler2D gbuffer_c3; // occlusion r, roughness g, metallic b, alpha unused (rgba8)
layout(location = 4) uniform sampler2D gbuffer_depth; // depth (d32)
#endif // DEFERRED

#ifdef FORWARD
// Shared Shader Data.
in shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
} fs_in;

#define texcoord fs_in.texcoord

layout(location = 0) uniform sampler2D sampler_base_color;
layout(location = 1) uniform sampler2D sampler_roughness_metallic;
layout(location = 2) uniform sampler2D sampler_occlusion;
layout(location = 3) uniform sampler2D sampler_normal;
layout(location = 4) uniform sampler2D sampler_emissive_color;
#endif // FORWARD


layout(location = 5) uniform samplerCube irradiance_map;
layout(location = 6) uniform samplerCube prefiltered_specular;
layout(location = 7) uniform sampler2D brdf_integration_lut;

layout(location = 8) uniform sampler2DArray shadow_map;

#ifdef FORWARD

// Uniform Buffer Model.
layout(binding = 2, std140) uniform model_data
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

// Uniform Buffer Material.
layout(binding = 3, std140) uniform material_data
{
    vec4  base_color;
    vec4  emissive_color; // this is a vec3, but there are annoying bugs with some drivers.
    float metallic;
    float roughness;

    bool base_color_texture;
    bool roughness_metallic_texture;
    bool occlusion_texture;
    bool packed_occlusion;
    bool normal_texture;
    bool emissive_color_texture;

    int   alpha_mode;
    float alpha_cutoff;
};

#endif // FORWARD

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

// Uniform Buffer Light Data.
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

#define MAX_SHADOW_CASCADES 4

// Uniform Buffer Shadow Data.
layout(binding = 6, std140) uniform shadow_data
{
    mat4  view_projection_matrices[MAX_SHADOW_CASCADES];
    float split_depth[MAX_SHADOW_CASCADES + 1]; // [0] is camera near, [cascade count] is camera far.
    vec4  far_planes;
    int   resolution;
    int   cascade_count;
    float shadow_cascade_interpolation_range;
    int sample_count;
    float slope_bias;
    float normal_bias;
    int filter_mode;
    float light_size;
};

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection)
{
    float z = depth * 2.0 - 1.0;

    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 direct = inverse_view_projection * clip;
    return direct.xyz / direct.w;
}

void populate_datapool()
{
#ifdef DEFERRED
    shader_datapool.base_color           = texture(gbuffer_c0, texcoord);
    shader_datapool.logarithmic_depth    = texture(gbuffer_depth, texcoord).r;
    shader_datapool.world_space_position = world_space_from_depth(shader_datapool.logarithmic_depth, texcoord, inverse_view_projection);

    shader_datapool.normal   = normalize(texture(gbuffer_c1, texcoord).rgb * 2.0 - 1.0);
    shader_datapool.emissive = texture(gbuffer_c2, texcoord).rgb;

    vec3 o_r_m                           = texture(gbuffer_c3, texcoord).rgb;
    shader_datapool.occlusion            = max(o_r_m.x, 0.089f);
    shader_datapool.perceptual_roughness = o_r_m.y;
    shader_datapool.metallic             = o_r_m.z;
#endif // DEFERRED
#ifdef FORWARD
    shader_datapool.base_color           = base_color_texture ? texture(sampler_base_color, texcoord) : base_color;
    shader_datapool.logarithmic_depth    = 0.0; // TODO Paul: Not set!
    shader_datapool.world_space_position = fs_in.position;

    {
        vec3 normal = normalize(fs_in.normal);
        if(!gl_FrontFacing)
            normal *= -1.0;
        vec3 dfdx = dFdx(fs_in.position);
        vec3 dfdy = dFdy(fs_in.position);
        if(!has_normals)
            normal = normalize(cross(dfdx, dfdy)); // approximation
        if(normal_texture)
        {
            vec3 tangent   = fs_in.tangent;
            vec3 bitangent = fs_in.bitangent;
            if(!has_tangents)
            {
                vec3 uv_dx = dFdx(vec3(texcoord, 0.0));
                vec3 uv_dy = dFdy(vec3(texcoord, 0.0));
                vec3 t_    = (uv_dy.y * dfdx - uv_dx.y * dfdy) / (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);
                tangent    = normalize(t_ - normal * dot(normal, t_));
                bitangent  = cross(normal, tangent);
            }

            mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normal);
            vec3 mapped_normal = normalize(texture(sampler_normal, texcoord).rgb * 2.0 - 1.0);
            normal = normalize(tbn * mapped_normal.rgb);
        }
        shader_datapool.normal   = normal;
    }

    shader_datapool.emissive = emissive_color_texture ? texture(sampler_emissive_color, texcoord).rgb : emissive_color.rgb;

    vec3 o_r_m                           = roughness_metallic_texture ? texture(sampler_roughness_metallic, texcoord).rgb : vec3(1.0, roughness, metallic);
    shader_datapool.occlusion            = max(o_r_m.x, 0.089f);
    shader_datapool.perceptual_roughness = o_r_m.y;
    shader_datapool.metallic             = o_r_m.z;
    if(!packed_occlusion)
        shader_datapool.occlusion = occlusion_texture ? texture(sampler_occlusion, texcoord).r : 1.0;

#endif // FORWARD

    shader_datapool.view_direction = normalize(camera_position.xyz - shader_datapool.world_space_position);
    shader_datapool.n_dot_v        = clamp(dot(shader_datapool.normal, shader_datapool.view_direction), 1e-5, 1.0 - 1e-5);


    shader_datapool.reflectance = 0.5; // TODO Paul: Make tweakable.
    shader_datapool.f0          = 0.16 * shader_datapool.reflectance * shader_datapool.reflectance * (1.0 - shader_datapool.metallic) + shader_datapool.base_color.rgb * shader_datapool.metallic;
    shader_datapool.real_albedo = shader_datapool.base_color.rgb * (1.0 - shader_datapool.metallic);
}

vec3 get_view_direction()
{
    return shader_datapool.view_direction;
}

float get_n_dot_v()
{
    return shader_datapool.n_dot_v;
}

mat4 get_camera_inverse_view_projection()
{
    return inverse_view_projection;
}

mat4 get_camera_view_matrix()
{
    return view;
}

bool is_shadow_step_enabled()
{
    return shadow_step_enabled;
}

vec3 get_camera_position()
{
    return camera_position.xyz;
};

float get_camera_near_plane()
{
    return camera_params.x;
};

float get_camera_far_plane()
{
    return camera_params.y;
};

bool is_debug_view_enabled()
{
    return debug_view_enabled;
}

bool should_debug_position()
{
    return debug_views_position;
}

bool should_debug_normal()
{
    return debug_views_normal;
}

bool should_debug_depth()
{
    return debug_views_depth;
}

bool should_debug_base_color()
{
    return debug_views_base_color;
}

bool should_debug_reflection_color()
{
    return debug_views_reflection_color;
}

bool should_debug_emission()
{
    return debug_views_emission;
}

bool should_debug_occlusion()
{
    return debug_views_occlusion;
}

bool should_debug_roughness()
{
    return debug_views_roughness;
}

bool should_debug_metallic()
{
    return debug_views_metallic;
}

bool should_show_cascades()
{
    return show_cascades;
}

bool should_draw_shadow_maps()
{
    return draw_shadow_maps;
}

vec3 get_directional_light_direction()
{
    return directional_direction.xyz;
}

vec3 get_directional_light_color()
{
    return directional_color.rgb;
}

float get_directional_light_intensity()
{
    return directional_intensity;
}

bool should_directional_light_cast_shadows()
{
    return directional_cast_shadows;
}

bool is_directional_light_valid()
{
    return directional_valid;
}

float get_skylight_intensity()
{
    return skylight_intensity;
}

bool is_skylight_valid()
{
    return skylight_valid;
}

mat4 get_shadow_camera_view_projection_matrix(in int cascade_id)
{
    return view_projection_matrices[cascade_id];
}

float get_shadow_cascades_split_depth(in int idx) // [0] is camera near, [cascade count] is camera far.
{
    return split_depth[idx];
}

float get_shadow_cascade_far_planes(in int cascade_id)
{
    return far_planes[cascade_id];
}

int get_shadow_resolution()
{
    return resolution;
}

int get_shadow_cascade_count()
{
    return cascade_count;
}

float get_shadow_cascade_interpolation_range()
{
    return shadow_cascade_interpolation_range;
}

int get_shadow_sample_count()
{
    return sample_count;
}

float get_shadow_slope_bias()
{
    return slope_bias;
}

float get_shadow_normal_bias()
{
    return normal_bias;
}

int get_shadow_filter_mode()
{
    return filter_mode;
}

float get_shadow_light_size()
{
    return light_size / float(resolution);
}

// debug views
void draw_debug_views()
{
    float depth                = get_logarithmic_depth();
    vec3 position              = get_world_space_position();
    vec4 base_color            = get_base_color();
    vec3 normal                = get_normal();
    float occlusion_factor     = get_occlusion();
    float perceptual_roughness = get_perceptual_roughness();
    float metallic             = get_metallic();
    vec3 emissive              = get_emissive();

    if(should_debug_position())
    {
        frag_color = vec4(position, 1.0);
        return;
    }
    if(should_debug_normal())
    {
        frag_color = vec4(normal, 1.0);
        return;
    }
    if(should_debug_depth())
    {
        float z_lin = (2.0 * get_camera_near_plane()) / (get_camera_far_plane() + get_camera_near_plane() - depth * (get_camera_far_plane() - get_camera_near_plane()));
        frag_color = vec4(vec3(z_lin), 1.0);
        return;
    }
    if(should_debug_base_color())
    {
        frag_color = vec4(vec3(1.0 - metallic) * base_color.rgb, 1.0);
        return;
    }
    if(should_debug_reflection_color())
    {
        frag_color = vec4(vec3(metallic) * base_color.rgb, 1.0);
        return;
    }
    if(should_debug_occlusion())
    {
        frag_color = vec4(vec3(occlusion_factor), 1.0);
        return;
    }
    if(should_debug_roughness())
    {
        frag_color = vec4(vec3(perceptual_roughness), 1.0);
        return;
    }
    if(should_debug_metallic())
    {
        frag_color = vec4(vec3(metallic), 1.0);
        return;
    }
    if(should_debug_emission())
    {
        frag_color = vec4(vec3(emissive), 1.0);
        return;
    }
}
#endif // LIGHTING

#ifdef GBUFFER_PREPASS
// Uniform Buffer Renderer.
layout(binding = 0, std140) uniform renderer_data
{
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    float camera_exposure;
    bool shadow_step_enabled;
};

// Uniform Buffer Model.
layout(binding = 2, std140) uniform model_data
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

// Uniform Buffer Material.
layout(binding = 3, std140) uniform material_data
{
    vec4  base_color;
    vec4  emissive_color; // this is a vec3, but there are annoying bugs with some drivers.
    float metallic;
    float roughness;

    bool base_color_texture;
    bool roughness_metallic_texture;
    bool occlusion_texture;
    bool packed_occlusion;
    bool normal_texture;
    bool emissive_color_texture;

    int   alpha_mode;
    float alpha_cutoff;
};

#ifdef VERTEX
// Vertex Input.
layout(location = 0) in vec3 vertex_data_position;
layout(location = 1) in vec3 vertex_data_normal;
layout(location = 2) in vec2 vertex_data_texcoord;
layout(location = 3) in vec4 vertex_data_tangent;

// Shared Shader Data.
out shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
} vs_out;

vec4 get_world_space_position()
{
    return model_matrix * vec4(vertex_data_position, 1.0);
}

void get_normal_tangent_bitangent(out vec3 normal, out vec3 tangent, out vec3 bitangent)
{
    if(has_normals)
        normal = normal_matrix * normalize(vertex_data_normal);
    if(has_tangents)
    {
        tangent = normal_matrix * normalize(vertex_data_tangent.xyz);

        if(has_normals)
        {
            bitangent = cross(normal, tangent);
            if(vertex_data_tangent.w == -1.0) // TODO Paul: Check this convention.
                bitangent *= -1.0;
        }
    }
}

void pass_shared_data(in vec4 world_position)
{
    // Perspective Division
    vs_out.position = world_position.xyz / world_position.w;

    // Texture Coordinates
    vs_out.texcoord = vertex_data_texcoord;

    // Normals, Tangents, Bitangents
    get_normal_tangent_bitangent(vs_out.normal, vs_out.tangent, vs_out.bitangent);
}
#endif // VERTEX

#ifdef FRAGMENT
// Output Textures GBuffer.
layout (location = 0) out vec4 gbuffer_color_target0; // base color rgba (rgba8)
layout (location = 1) out vec4 gbuffer_color_target1; // normal rgb, alpha unused (rgb10a2)
layout (location = 2) out vec4 gbuffer_color_target2; // emissive rgb, alpha unused (rgba8)
layout (location = 3) out vec4 gbuffer_color_target3; // occlusion r, roughness g, metallic b, alpha unused (rgba8)

// Shared Shader Data.
in shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
} fs_in;

// Texture Samplers.
layout (location = 0) uniform sampler2D sampler_base_color;
layout (location = 1) uniform sampler2D sampler_roughness_metallic;
layout (location = 2) uniform sampler2D sampler_occlusion;
layout (location = 3) uniform sampler2D sampler_normal;
layout (location = 4) uniform sampler2D sampler_emissive_color;

#include <common_constants_and_functions.glsl>

vec4 get_base_color()
{
    vec4 color = base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;

    if(alpha_mode == 3)
        alpha_dither(gl_FragCoord.xy, sqrt(color.a));

    return color;
}

vec3 get_emissive()
{
    return emissive_color_texture ? texture(sampler_emissive_color, fs_in.texcoord).rgb : emissive_color.rgb;
}

vec3 get_occlusion_roughness_metallic()
{
    vec3 o_r_m = roughness_metallic_texture ? texture(sampler_roughness_metallic, fs_in.texcoord).rgb : vec3(1.0, roughness, metallic);
    if(packed_occlusion)
        return o_r_m;

    float occlusion = occlusion_texture ? texture(sampler_occlusion, fs_in.texcoord).r : 1.0;
    o_r_m.r = occlusion;

    return o_r_m;
}

vec3 get_normal()
{
    vec3 normal = normalize(fs_in.normal);
    vec3 dfdx = dFdx(fs_in.position);
    vec3 dfdy = dFdy(fs_in.position);
    if(!has_normals)
        normal = normalize(cross(dfdx, dfdy)); // approximation
    if(normal_texture)
    {
        vec3 tangent   = fs_in.tangent;
        vec3 bitangent = fs_in.bitangent;
        if(!has_tangents)
        {
            vec2 uv_dx = dFdx(vec2(fs_in.texcoord));
            vec2 uv_dy = dFdy(vec2(fs_in.texcoord));
            vec3 t_    = (uv_dy.y * dfdx - uv_dx.y * dfdy) / (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);
            tangent    = normalize(t_ - normal * dot(normal, t_));
            bitangent  = cross(normal, tangent);
        }

        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normal);
        vec3 mapped_normal = normalize(texture(sampler_normal, fs_in.texcoord).rgb * 2.0 - 1.0);
        normal = normalize(tbn * mapped_normal.rgb);
    }
    if(!gl_FrontFacing)
        normal *= -1.0;
    return normal * 0.5 + 0.5;
}

void populate_gbuffer()
{
    gbuffer_color_target0 = vec4(get_base_color());
    gbuffer_color_target1 = vec4(get_normal(), 1.0);
    gbuffer_color_target2 = vec4(get_emissive(), 1.0);
    gbuffer_color_target3 = vec4(get_occlusion_roughness_metallic(), 1.0);
}

#endif // FRAGMENT

mat4 get_camera_view_projection_matrix()
{
    return view_projection_matrix;
}

#endif // GBUFFER_PREPASS

#endif // MANGO_COMMON_STATE_GLSL
