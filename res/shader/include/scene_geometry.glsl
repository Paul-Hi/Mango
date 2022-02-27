#ifndef MANGO_SCENE_GEOMETRY_GLSL
#define MANGO_SCENE_GEOMETRY_GLSL

#include <bindings.glsl>
#include <common_constants_and_functions.glsl>

#ifdef VERTEX

layout(location = VERTEX_INPUT_POSITION) in vec3 vertex_data_position;
layout(location = VERTEX_INPUT_NORMAL) in vec3 vertex_data_normal;
layout(location = VERTEX_INPUT_TEXCOORD) in vec2 vertex_data_texcoord;
layout(location = VERTEX_INPUT_TANGENT) in vec4 vertex_data_tangent;
layout(location = VERTEX_INPUT_DRAW_ID) in ivec2 vertex_data_draw_id;

out shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    flat ivec2 draw_id; // (model_data index, material_data index)
} vs_out;

#define BIND_CAMERA_DATA_BUFFER
#define BIND_MODEL_DATA_BUFFER

#include <binding_data.glsl>

per_model_data get_draw_model_data()
{
    int model_id = vertex_data_draw_id.x;
    per_model_data data = model_data_array[model_id];
    return data;
}

mat4 get_model_matrix()
{
    per_model_data data = get_draw_model_data();
    return data.model_matrix;
}

void get_normal_tangent_bitangent(out vec3 normal, out vec3 tangent, out vec3 bitangent)
{
    per_model_data data = get_draw_model_data();
    if(data.has_normals)
        normal = data.normal_matrix * normalize(vertex_data_normal);
    if(data.has_tangents)
    {
        tangent = data.normal_matrix * normalize(vertex_data_tangent.xyz);

        if(data.has_normals)
        {
            bitangent = cross(normal, tangent);
            if(vertex_data_tangent.w == -1.0)
                bitangent *= -1.0;
        }
    }
}

#endif // VERTEX

#ifdef GBUFFER_FRAGMENT

layout(location = GBUFFER_OUTPUT_TARGET0) out vec4 gbuffer_color_target0; // base color rgba (rgba8)
layout(location = GBUFFER_OUTPUT_TARGET1) out vec4 gbuffer_color_target1; // normal rgb, alpha unused (rgb10a2)
layout(location = GBUFFER_OUTPUT_TARGET2) out vec4 gbuffer_color_target2; // emissive rgb, alpha unused (rgba32f)
layout(location = GBUFFER_OUTPUT_TARGET3) out vec4 gbuffer_color_target3; // occlusion r, roughness g, metallic b, alpha unused (rgba8)

in shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    flat ivec2 draw_id; // (model_data index, material_data index)
} fs_in;

layout(binding = GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR) uniform sampler2D sampler_base_color; // texture "texture_base_color"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC) uniform sampler2D sampler_roughness_metallic; // texture "texture_roughness_metallic"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_OCCLUSION) uniform sampler2D sampler_occlusion; // texture "texture_occlusion"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_NORMAL) uniform sampler2D sampler_normal; // texture "texture_normal"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR) uniform sampler2D sampler_emissive_color; // texture "texture_emissive_color"

#define BIND_MODEL_DATA_BUFFER
#define BIND_MATERIAL_DATA_BUFFER

#include <binding_data.glsl>

per_model_data get_draw_model_data()
{
    int model_id = fs_in.draw_id.x;
    per_model_data data = model_data_array[model_id];
    return data;
}

per_material_data get_draw_material_data()
{
    int material_id = fs_in.draw_id.y;
    per_material_data data = material_data_array[material_id];
    return data;
}

vec4 get_base_color()
{
    per_material_data data = get_draw_material_data();
    vec4 color = data.base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : data.base_color;
    if(data.alpha_mode == 2 && color.a <= data.alpha_cutoff)
        discard;

    if(data.alpha_mode == 1 && alpha_dither(gl_FragCoord.xy, sqrt(color.a)))
        discard;

    return color;
}

vec3 get_emissive()
{
    per_material_data data = get_draw_material_data();
    return data.emissive_intensity * (data.emissive_color_texture ? texture(sampler_emissive_color, fs_in.texcoord).rgb : data.emissive_color.rgb) * data.emissive_intensity;
}

vec3 get_occlusion_roughness_metallic()
{
    per_material_data data = get_draw_material_data();
    vec3 o_r_m = data.roughness_metallic_texture ? texture(sampler_roughness_metallic, fs_in.texcoord).rgb : vec3(1.0, data.roughness, data.metallic);
    if(data.packed_occlusion)
        return o_r_m;

    float occlusion = data.occlusion_texture ? texture(sampler_occlusion, fs_in.texcoord).r : 1.0;
    o_r_m.r = occlusion;

    return o_r_m;
}

vec3 get_normal()
{
    per_model_data data_model = get_draw_model_data();
    per_material_data data_material = get_draw_material_data();
    vec3 normal = normalize(fs_in.normal);
    vec3 dfdx = dFdx(fs_in.position);
    vec3 dfdy = dFdy(fs_in.position);
    if(!data_model.has_normals)
        normal = normalize(cross(dfdx, dfdy)); // approximation
    if(data_material.normal_texture)
    {
        vec3 tangent   = fs_in.tangent;
        vec3 bitangent = fs_in.bitangent;
        if(!data_model.has_tangents)
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
    return normal;
}

#endif // GBUFFER_FRAGMENT

#ifdef FORWARD_LIGHTING_FRAGMENT

in shared_data
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    flat ivec2 draw_id; // (model_data index, material_data index)
} fs_in;

out vec4 frag_color;

layout(binding = GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR) uniform sampler2D sampler_base_color; // texture "texture_base_color"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_ROUGHNESS_METALLIC) uniform sampler2D sampler_roughness_metallic; // texture "texture_roughness_metallic"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_OCCLUSION) uniform sampler2D sampler_occlusion; // texture "texture_occlusion"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_NORMAL) uniform sampler2D sampler_normal; // texture "texture_normal"
layout(binding = GEOMETRY_TEXTURE_SAMPLER_EMISSIVE_COLOR) uniform sampler2D sampler_emissive_color; // texture "texture_emissive_color"

layout(binding = IBL_SAMPLER_IRRADIANCE_MAP) uniform samplerCube sampler_irradiance_map; // texture "texture_irradiance_map"
layout(binding = IBL_SAMPLER_RADIANCE_MAP) uniform samplerCube sampler_radiance_map; // texture "texture_radiance_map"
layout(binding = IBL_SAMPLER_LOOKUP) uniform sampler2D sampler_brdf_integration_lut; // texture "texture_brdf_integration_lut"

layout(binding = SAMPLER_SHADOW_SHADOW_MAP) uniform sampler2DArrayShadow sampler_shadow_shadow_map; // texture "texture_shadow_map_comp"
layout(binding = SAMPLER_SHADOW_MAP) uniform sampler2DArray sampler_shadow_map; // texture "texture_shadow_map"

#define BIND_RENDERER_DATA_BUFFER
#define BIND_CAMERA_DATA_BUFFER
#define BIND_MODEL_DATA_BUFFER
#define BIND_MATERIAL_DATA_BUFFER
#define BIND_LIGHT_DATA_BUFFER
#define BIND_SHADOW_DATA_BUFFER

#include <binding_data.glsl>

per_model_data get_draw_model_data()
{
    int model_id = fs_in.draw_id.x;
    per_model_data data = model_data_array[model_id];
    return data;
}

per_material_data get_draw_material_data()
{
    int material_id = fs_in.draw_id.y;
    per_material_data data = material_data_array[material_id];
    return data;
}

vec4 get_base_color()
{
    per_material_data data = get_draw_material_data();
    vec4 color = data.base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : data.base_color;
    if(data.alpha_mode == 2 && color.a <= data.alpha_cutoff)
        discard;

    if(data.alpha_mode == 1 && alpha_dither(gl_FragCoord.xy, sqrt(color.a)))
        discard;

    return color;
}

vec3 get_emissive()
{
    per_material_data data = get_draw_material_data();
    return data.emissive_intensity * (data.emissive_color_texture ? texture(sampler_emissive_color, fs_in.texcoord).rgb : data.emissive_color.rgb) * data.emissive_intensity;
}

vec3 get_occlusion_roughness_metallic()
{
    per_material_data data = get_draw_material_data();
    vec3 o_r_m = data.roughness_metallic_texture ? texture(sampler_roughness_metallic, fs_in.texcoord).rgb : vec3(1.0, data.roughness, data.metallic);
    if(data.packed_occlusion)
        return o_r_m;

    float occlusion = data.occlusion_texture ? texture(sampler_occlusion, fs_in.texcoord).r : 1.0;
    o_r_m.r = occlusion;

    return o_r_m;
}

vec3 get_normal()
{
    per_model_data data_model = get_draw_model_data();
    per_material_data data_material = get_draw_material_data();
    vec3 normal = normalize(fs_in.normal);
    vec3 dfdx = dFdx(fs_in.position);
    vec3 dfdy = dFdy(fs_in.position);
    if(!data_model.has_normals)
        normal = normalize(cross(dfdx, dfdy)); // approximation
    if(data_material.normal_texture)
    {
        vec3 tangent   = fs_in.tangent;
        vec3 bitangent = fs_in.bitangent;
        if(!data_model.has_tangents)
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
    return normal;
}

float get_logarithmic_depth()
{
    return gl_FragDepth;
}

void draw_debug_views()
{
    float depth                = get_logarithmic_depth();
    vec3 position              = fs_in.position;
    vec4 base_color            = get_base_color();
    vec3 normal                = get_normal();
    vec3 o_r_m                 = get_occlusion_roughness_metallic();
    float occlusion_factor     = o_r_m.r;
    float perceptual_roughness = o_r_m.g;
    float metallic             = o_r_m.b;
    vec3 emissive              = get_emissive();

    if(position_debug_view)
    {
        frag_color = vec4(position, 1.0);
        return;
    }
    if(normal_debug_view)
    {
        frag_color = vec4(normal, 1.0);
        return;
    }
    if(depth_debug_view)
    {
        float z_lin = linearize_depth(depth, camera_near, camera_far) / camera_far;
        frag_color = vec4(vec3(z_lin), 1.0);
        return;
    }
    if(base_color_debug_view)
    {
        frag_color = vec4(vec3(1.0 - metallic) * base_color.rgb, 1.0);
        return;
    }
    if(reflection_color_debug_view)
    {
        frag_color = vec4(vec3(metallic) * base_color.rgb, 1.0);
        return;
    }
    if(occlusion_debug_view)
    {
        frag_color = vec4(vec3(occlusion_factor), 1.0);
        return;
    }
    if(roughness_debug_view)
    {
        frag_color = vec4(vec3(perceptual_roughness), 1.0);
        return;
    }
    if(metallic_debug_view)
    {
        frag_color = vec4(vec3(metallic), 1.0);
        return;
    }
    if(emission_debug_view)
    {
        frag_color = vec4(vec3(emissive), 1.0);
        return;
    }
}

#endif // FORWARD_LIGHTING_FRAGMENT

#endif // MANGO_SCENE_GEOMETRY_GLSL