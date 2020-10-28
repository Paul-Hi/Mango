#version 430 core

const float PI = 3.1415926535897932384626433832795;
const float INV_PI = 1.0 / PI;

#define saturate(x) clamp(x, 0.0, 1.0)
#define max_cascades 4

// Output Texture HDR
out vec4 hdr_out;

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
layout (location = 0, binding = 0) uniform sampler2D sampler_base_color;
layout (location = 1, binding = 1) uniform sampler2D sampler_roughness_metallic;
layout (location = 2, binding = 2) uniform sampler2D sampler_occlusion;
layout (location = 3, binding = 3) uniform sampler2D sampler_normal;
layout (location = 4, binding = 4) uniform sampler2D sampler_emissive_color;

layout(location = 5, binding = 5) uniform samplerCube irradiance_map;
layout(location = 6, binding = 6) uniform samplerCube prefiltered_specular;
layout(location = 7, binding = 7) uniform sampler2D brdf_integration_lut;

layout(location = 8, binding = 8) uniform sampler2DArray shadow_map;

// Uniform Buffer Renderer.
layout(binding = 0, std140) uniform renderer_data
{
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    float camera_exposure;
};

// Uniform Buffer Model.
layout(binding = 1, std140) uniform model_data
{
    mat4 model_matrix;
    mat3 normal_matrix;
    bool has_normals;
    bool has_tangents;
};

// Uniform Buffer Material.
layout(binding = 2, std140) uniform material_data
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

// Uniform Buffer Shadow.
layout(binding = 4, std140) uniform shadow_data
{
    mat4  view_projection_matrices[max_cascades];
    float split_depth[max_cascades + 1];
    vec4  far_planes;
    int   resolution;
    int   cascade_count;
    float shadow_cascade_interpolation_range;
    float maximum_penumbra;
} shadow_map_data;

vec4 get_base_color()
{
    vec4 color = base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : base_color;

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
            vec3 uv_dx = dFdx(vec3(fs_in.texcoord, 0.0));
            vec3 uv_dy = dFdy(vec3(fs_in.texcoord, 0.0));
            vec3 t_    = (uv_dy.y * dfdx - uv_dx.y * dfdy) / (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);
            tangent    = normalize(t_ - normal * dot(normal, t_));
            bitangent  = cross(normal, tangent);
        }

        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normal);
        vec3 mapped_normal = normalize(texture(sampler_normal, fs_in.texcoord).rgb * 2.0 - 1.0);
        normal = normalize(tbn * mapped_normal.rgb);
    }
    return normal;
}

float D_GGX(in float n_dot_h, in float roughness);
float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness);
vec3 F_Schlick(in float dot, in vec3 f0, in float f90);
vec3 F_Schlick_roughness(in float dot, in vec3 f0, in float roughness);
float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness);

vec3 calculate_image_based_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor);
vec3 calculate_directional_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor, in vec3 world_pos);

vec3 get_specular_dominant_direction(in vec3 normal, in vec3 reflection, in float roughness);

// interpolation_mode: 0 -> no interpolation | 1 -> interpolation from cascade_id to cascade_id + 1 - shadow_map_data.shadow_cascade_interpolation_range | 2 -> interpolation from cascade_id - 1  + shadow_map_data.shadow_cascade_interpolation_range to cascade_id
int compute_cascade_id(in float view_depth, out float interpolation_factor, out int interpolation_mode) // xy texcoords, z depth
{
    interpolation_mode = 0;
    int cascade_id = 0;
    int num_cascades = shadow_map_data.cascade_count;
    for(int i = 1; i <= num_cascades; ++i)
    {
        float i_value = shadow_map_data.split_depth[i];
        if(view_depth <= i_value)
        {
            cascade_id = i - 1;
            float i_minus_one_value = shadow_map_data.split_depth[i - 1];
            float mid = (i_value + i_minus_one_value) * 0.5;
            if(view_depth > mid && view_depth > i_value - shadow_map_data.shadow_cascade_interpolation_range)
            {
                interpolation_mode = 1;
                interpolation_factor = (i_value - view_depth) / shadow_map_data.shadow_cascade_interpolation_range;
            }
            if(view_depth < mid && view_depth < i_minus_one_value + shadow_map_data.shadow_cascade_interpolation_range)
            {
                interpolation_mode = 2;
                interpolation_factor = (view_depth - i_minus_one_value) / shadow_map_data.shadow_cascade_interpolation_range;
            }
            break;
        }
    }

    interpolation_factor = (1.0 - interpolation_factor);

    if(cascade_id == 0 && interpolation_mode == 2)
        interpolation_mode = 0;
    if(cascade_id == num_cascades - 1 && interpolation_mode == 1)
        interpolation_mode = 0;

    return cascade_id;
}

float linearize_depth(in float d, in float near, in float far)
{
    float depth = 2.0 * d - 1.0;
    return 2.0 * near * far / (far + near - depth * (far - near));
}


float interleaved_gradient_noise(in vec2 screen_pos)
{
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screen_pos, magic.xy)));
}

vec2 sample_vogel_disc(in int idx, in int num_samples, in float phi)
{
    float golden_angle = 2.4;

    float r = sqrt(float(idx) + 0.5) / sqrt(float(num_samples));
    float theta = float(idx) * golden_angle + phi;
    float sine = sin(theta);
    float cosine = cos(theta);

    return vec2(cosine * r, sine * r);
}

float sample_blocker_distance(in vec3 shadow_coords, in int cascade_id, in sampler2DArray lookup, in int sample_count, in float gradient_noise, in float light_size)
{
    float avg_blocker_depth = 0.0;
    int blocker_count = 0;
    float max_width = (shadow_map_data.maximum_penumbra * 4.0) / shadow_map_data.resolution;
    float d_l = linearize_depth(shadow_coords.z, 1e-5, shadow_map_data.far_planes[cascade_id]);

    for(int i = 0; i < sample_count; ++i)
    {
        vec2 sample_uv = sample_vogel_disc(i, sample_count, gradient_noise);
        sample_uv = shadow_coords.xy + sample_uv * max_width;

        float z = texture(lookup, vec3(sample_uv, cascade_id)).x;
        float c_l = linearize_depth(z, 1e-5, shadow_map_data.far_planes[cascade_id]);
        if(c_l < d_l)
        {
            avg_blocker_depth += z;
            blocker_count++;
        }
    }

    if(blocker_count > 0)
    {
        avg_blocker_depth /= float(blocker_count);
        float c_l = linearize_depth(avg_blocker_depth, 1e-5, shadow_map_data.far_planes[cascade_id]);
        return light_size * (d_l - c_l) / c_l;
    }
    else
        return 0.0;
}

float pcss(in vec3 shadow_coords, in int cascade_id, in sampler2DArray lookup)
{
    int num_samples = int(shadow_map_data.maximum_penumbra * 3.0);
    int num_blocker_samples = int(shadow_map_data.maximum_penumbra * 1.5);
    float gradient_noise = interleaved_gradient_noise(gl_FragCoord.xy);
    float penumbra = saturate(sample_blocker_distance(shadow_coords, cascade_id, lookup, num_blocker_samples, gradient_noise, 2.0));
    float shadow = 0.0;
    float max_penumbra = shadow_map_data.maximum_penumbra / shadow_map_data.resolution;
    float d_l = linearize_depth(shadow_coords.z, 1e-5, shadow_map_data.far_planes[cascade_id]);
    num_samples += int(penumbra * 12.0);

    for(int i = 0; i < num_samples; ++i)
    {
        vec2 sample_uv = sample_vogel_disc(i, num_samples, gradient_noise);
        sample_uv = shadow_coords.xy + sample_uv * max(max_penumbra * penumbra, 1.0 / shadow_map_data.resolution);

        float z = texture(lookup, vec3(sample_uv, cascade_id)).x;
        float c_l = linearize_depth(z, 1e-5, shadow_map_data.far_planes[cascade_id]);
        shadow += (c_l < d_l) ? 0.0 : 1.0;
    }
    shadow /= float(num_samples);
    return shadow;
}

float directional_shadow(in sampler2DArray lookup, in vec3 world_pos, in int cascade_id, in float interpolation_factor, in int interpolation_mode)
{

    vec3 bias = get_normal() * 0.1;
    world_pos += bias;
    float shadow = 1.0;


    if(interpolation_mode == 0)
    {
        vec4 projected = shadow_map_data.view_projection_matrices[cascade_id] * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;
        shadow_coords = shadow_coords * 0.5 + 0.5;

        if(shadow_coords.z > 1.0)
            return shadow;

        shadow = pcss(shadow_coords, cascade_id, lookup);
    }
    else if(interpolation_mode == 1)
    {
        vec4 projected = shadow_map_data.view_projection_matrices[cascade_id] * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;
        shadow_coords = shadow_coords * 0.5 + 0.5;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = pcss(shadow_coords, cascade_id, lookup);

        vec4 projected2 = shadow_map_data.view_projection_matrices[cascade_id + 1] * vec4(world_pos, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;
        shadow_coords2 = shadow_coords2 * 0.5 + 0.5;

        if(shadow_coords2.z > 1.0)
            return pcss(shadow_coords, cascade_id, lookup);

        float pcss_shadows2 = pcss(shadow_coords2, cascade_id + 1, lookup);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }
    else if(interpolation_mode == 2)
    {
        vec4 projected = shadow_map_data.view_projection_matrices[cascade_id] * vec4(world_pos, 1.0);
        vec3 shadow_coords = projected.xyz / projected.w;
        shadow_coords = shadow_coords * 0.5 + 0.5;

        if(shadow_coords.z > 1.0)
            return shadow;

        float pcss_shadows = pcss(shadow_coords, cascade_id, lookup);

        vec4 projected2 = shadow_map_data.view_projection_matrices[cascade_id - 1] * vec4(world_pos, 1.0);
        vec3 shadow_coords2 = projected2.xyz / projected2.w;
        shadow_coords2 = shadow_coords2 * 0.5 + 0.5;

        if(shadow_coords2.z > 1.0)
            return pcss(shadow_coords, cascade_id, lookup);

        float pcss_shadows2 = pcss(shadow_coords2, cascade_id - 1, lookup);

        shadow = mix(pcss_shadows, pcss_shadows2, interpolation_factor * 0.5);
    }

    return shadow;
}

void main()
{
    vec4 base_color                      = get_base_color();
    vec3 normal                          = get_normal();
    vec3 occlusion_roughness_metallic    = get_occlusion_roughness_metallic();
    float occlusion_factor               = occlusion_roughness_metallic.r;
    float perceptual_roughness           = occlusion_roughness_metallic.g;
    float metallic                       = occlusion_roughness_metallic.b;
    vec3 emissive                        = get_emissive();
    float reflectance                    = 1.0; // TODO Paul: Make tweakable.

    vec3 f0          = 0.16 * reflectance * reflectance * (1.0 - metallic) + base_color.rgb * metallic;
    vec3 real_albedo = base_color.rgb * (1.0 - metallic);

    vec3 view_dir = normalize(camera_position.xyz - fs_in.position);
    float n_dot_v = clamp(dot(normal, view_dir), 1e-5, 1.0 - 1e-5);

    vec3 lighting = vec3(0.0);

    // environment
    lighting += calculate_image_based_light(real_albedo, n_dot_v, view_dir, normal, perceptual_roughness, f0, occlusion_factor);

    // lights
    lighting += calculate_directional_light(real_albedo, n_dot_v, view_dir, normal, perceptual_roughness, f0, occlusion_factor, fs_in.position);

    lighting += emissive * 50000; // TODO Paul: Remove hardcoded intensity for all emissive values -.-

    hdr_out = vec4(lighting * base_color.a, base_color.a); // Premultiplied alpha?
}

vec3 calculate_image_based_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor)
{
    const float DFG_TEXTURE_SIZE = 256.0; // TODO Paul: Hardcoded -.-

    n_dot_v = clamp(n_dot_v , 0.5 / DFG_TEXTURE_SIZE, 1.0 - 0.5 / DFG_TEXTURE_SIZE);
    vec3 dfg = textureLod(brdf_integration_lut, saturate(vec2(n_dot_v, perceptual_roughness)), 0.0).xyz;
    // irradiance
    vec3 irradiance  = texture(irradiance_map, normal).rgb;
    vec3 diffuse     = irradiance * real_albedo;
    vec3 diffuse_ibl = diffuse;

    // specular
    const float MAX_REFLECTION_LOD = 10.0; // TODO Paul: Hardcoded -.-

    vec3 refl              = -normalize(reflect(view_dir, normal));
    vec3 dominant_refl     = get_specular_dominant_direction(normal, refl, perceptual_roughness);
    float mip_index        = perceptual_roughness * MAX_REFLECTION_LOD;
    vec3 prefiltered_color = textureLod(prefiltered_specular, dominant_refl, mip_index).rgb;
    vec3 specular_ibl      = prefiltered_color * mix(dfg.xxx, dfg.yyy, f0);

    vec3 energy_compensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
    specular_ibl *= energy_compensation;

    return (diffuse_ibl * occlusion_factor + specular_ibl) * ambient_intensity;
}

vec3 calculate_directional_light(in vec3 real_albedo, in float n_dot_v, in vec3 view_dir, in vec3 normal, in float perceptual_roughness, in vec3 f0, in float occlusion_factor, in vec3 world_pos)
{
    float light_intensity = directional_intensity;
    if(light_intensity < 1e-5)
        return vec3(0.0);
    vec3 light_dir        = normalize(directional_direction.xyz);
    vec3 light_col        = directional_color.rgb;
    float roughness       = (perceptual_roughness * perceptual_roughness);

    // adjust roughness to approximate small disk
    float lightRoughness = 0.1 * 696340.0 / 14960000.0; // sun radius / sun distance * 0.1 -> some approximation.
    float specular_roughness = saturate(lightRoughness + roughness);

    vec3 lighting = vec3(0.0);

    vec3 halfway  = normalize(light_dir + view_dir);
    float n_dot_l = saturate(dot(normal, light_dir));
    float n_dot_h = saturate(dot(normal, halfway));
    float l_dot_h = saturate(dot(light_dir, halfway));

    float D = D_GGX(n_dot_h, specular_roughness);
    vec3 F  = F_Schlick(l_dot_h, f0, 1.0);
    float V = V_SmithGGXCorrelated(n_dot_v, n_dot_l, specular_roughness);

    // Fr energy compensation
    vec3 Fr = D * V * F * (1.0 / PI);

    vec3 Fd = real_albedo * Fd_BurleyRenormalized(n_dot_v, n_dot_l, l_dot_h, roughness) * (1.0 / PI);

    vec3 diffuse = n_dot_l * Fd;
    vec3 specular = n_dot_l * Fr;

    lighting += (diffuse * occlusion_factor + specular) * light_col * light_intensity;

    if(cast_shadows)
    {
        vec4 view_pos = view * vec4(world_pos, 1.0);
        int interpolation_mode;
        float interpolation_factor;
        int cascade_id = compute_cascade_id(abs(view_pos.z), interpolation_factor, interpolation_mode);
        float shadow = cast_shadows ? directional_shadow(shadow_map, world_pos, cascade_id, interpolation_factor, interpolation_mode) : 1.0;
        lighting = lighting * shadow;
        if(show_cascades)
        {
            if(cascade_id == 0) lighting.gb *= vec2(0.25);
            if(cascade_id == 1) lighting.rb *= vec2(0.25);
            if(cascade_id == 2) lighting.rg *= vec2(0.25);
            if(cascade_id == 3) lighting.b  *= 0.125;
            if(interpolation_mode != 0) lighting *= 0.5;
        }
    }

    return lighting;
}

//
// see https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf.
//

float D_GGX(in float n_dot_h, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float f = (n_dot_h * a_sqr - n_dot_h) * n_dot_h + 1.0;
    // Gets divided by pi later on.
    return a_sqr / (f * f);
}

float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float GGXL = n_dot_v * sqrt((-n_dot_l * a_sqr + n_dot_l) * n_dot_l + a_sqr);
    float GGXV = n_dot_l * sqrt((-n_dot_v * a_sqr + n_dot_v) * n_dot_v + a_sqr);
    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

vec3 F_Schlick(in float dot, in vec3 f0, in float f90) // can be optimized
{
    return f0 + (vec3(f90) - f0) * pow(saturate(1.0 - dot), 5.0);
}

vec3 F_Schlick_roughness(in float dot, in vec3 f0, in float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(saturate(1.0 - dot), 5.0);
}

float Fd_BurleyRenormalized(in float n_dot_v, in float n_dot_l, in float l_dot_h, in float roughness) // normalized Frostbyte version
{
    float energy_bias = mix(0.0, 0.5, roughness);
    float energy_factor = mix(1.0, 1.0 / 1.51, roughness);
    float f90 = energy_bias + 2.0 * l_dot_h * l_dot_h * roughness;
    vec3 f0 = vec3(1.0);
    float light_scatter = F_Schlick(n_dot_l, f0, f90).x;
    float view_scatter = F_Schlick(n_dot_v, f0, f90).x;
    // Gets divided by pi later on.
    return energy_factor * light_scatter * view_scatter;
}

vec3 get_specular_dominant_direction(in vec3 normal, in vec3 reflection, in float roughness)
{
    float smoothness = saturate(1.0 - roughness);
    float lerp_f = smoothness * (sqrt(smoothness) + roughness);
    return mix(normal, reflection, lerp_f);
}
