#version 430 core

#define PI 3.14159265358979323846f

#define saturate(x) clamp(x, 0.0f, 1.0f)

out vec4 frag_color;

in vec2 texcoord;

layout (location = 0) uniform mat4 u_inverse_view_projection;
layout (location = 1) uniform vec3 u_camera_position;

layout(location = 2, binding = 0) uniform sampler2D gbuffer_c0;
layout(location = 3, binding = 1) uniform sampler2D gbuffer_c1;
layout(location = 4, binding = 2) uniform sampler2D gbuffer_c2;
layout(location = 5, binding = 3) uniform sampler2D gbuffer_depth;

float D_GGX(in float n_dot_h, in float roughness);
float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness);
vec3 F_Schlick(in float dot, in vec3 f0, in float f90);
float Fd_BurleyRenormalized(float n_dot_v, float n_dot_l, float l_dot_h, float roughness);

float linearize_depth(in float depth)
{
    float z_near = 0.01f; // TODO: Need Uniform
    float z_far  = 100.0f; // TODO: Need Uniform
    return (2.0f * z_near) / (z_far + z_near - depth * (z_far - z_near));
}

vec3 world_space_from_depth(in float depth, in vec2 uv, in mat4 inverse_view_projection)
{
    float z = depth * 2.0f - 1.0f;

    vec4 clip = vec4(uv * 2.0f - 1.0f, z, 1.0f);
    vec4 direct = inverse_view_projection * clip;
    return direct.xyz / direct.w;
}

void main()
{
    float depth = texture(gbuffer_depth, texcoord).r;
    if(depth >= 1.0f) discard;

    vec3 light_pos             = vec3(-1.0f, 1.0f, 10.0f); // hardcoded
    float light_intensity      = 1.0f; // hardcoded
    vec3 light_col             = vec3(1.0f) * light_intensity; // hardcoded
    vec3 position              = world_space_from_depth(depth, texcoord, u_inverse_view_projection);
    vec3 light_dir             = normalize(light_pos - position);
    vec4 b1                    = texture(gbuffer_c0, texcoord);
    vec4 b2                    = texture(gbuffer_c1, texcoord);
    vec4 b3                    = texture(gbuffer_c2, texcoord);
    vec3 albedo                = b1.rgb;
    vec3 normal                = normalize(b2.xyz * 2.0f - 1.0f);
    float roughness            = b2.a;
    float metallic             = b3.r;

    vec3 real_albedo  = albedo * (1.0f - metallic);
    float reflectance = 1.0f;
    vec3 f0           = 0.16f * reflectance * reflectance * (1.0f - metallic) + albedo * metallic;

    vec3 view_dir = normalize(u_camera_position - position);
    float n_dot_v = abs(dot(normal, view_dir)) + 1e-5f;


    vec3 lighting = vec3(0.0f);

    vec3 ambient = 0.02f * real_albedo;
    lighting += ambient;

    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    vec3 halfway = normalize(light_dir + view_dir);
    float n_dot_l = saturate(dot(normal, light_dir));
    float n_dot_h = saturate(dot(normal, halfway));
    float l_dot_h = saturate(dot(light_dir, halfway));

    float D = D_GGX(n_dot_h, roughness);
    vec3 F  = F_Schlick(l_dot_h, f0, 1.0f);
    float V = V_SmithGGXCorrelated(n_dot_v, n_dot_l, roughness);

    // Fr energy compensation
    vec3 Fr = D * V * F / PI;

    vec3 Fd = real_albedo * Fd_BurleyRenormalized(n_dot_v, n_dot_l, l_dot_h, roughness) / PI;

    diffuse += n_dot_l * Fd;
    specular += n_dot_l * Fr;

    lighting += ((diffuse + specular) * light_col);


    // improvising gamma correction
    lighting = pow(lighting, vec3(1.0f / 2.2f));

    frag_color = vec4(saturate(lighting), 1.0f);
}


//
// see https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf.
//

float D_GGX(in float n_dot_h, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float f = (n_dot_h * a_sqr - n_dot_h) * n_dot_h + 1.0f;
    // Gets divided by pi later on.
    return a_sqr / (f * f);
}

float V_SmithGGXCorrelated(in float n_dot_v, in float n_dot_l, in float roughness) // can be optimized
{
    float a_sqr = roughness * roughness;
    float GGXV = n_dot_l * sqrt((-n_dot_v * a_sqr + n_dot_v) * n_dot_v + a_sqr);
    float GGXL = n_dot_v * sqrt((-n_dot_l * a_sqr + n_dot_l) * n_dot_l + a_sqr);
    return 0.5f / (GGXV + GGXL);
}

vec3 F_Schlick(in float dot, in vec3 f0, in float f90) // can be optimized
{
    return f0 + (vec3(f90) - f0) * pow(1.0f - dot, 5.0f);
}

float Fd_BurleyRenormalized(float n_dot_v, float n_dot_l, float l_dot_h, float roughness) // normalized Frostbyte version
{
    float energyBias = mix(0.0f, 0.5f, roughness);
    float energyFactor = mix(1.0f, 1.0f / 1.51f, roughness);
    float f90 = energyBias + 2.0f * roughness * l_dot_h * l_dot_h;
    vec3 f0 = vec3(1.0f);
    float lightScatter = F_Schlick(n_dot_l, f0, f90).r;
    float viewScatter = F_Schlick(n_dot_v, f0, f90).r;
    // Gets divided by pi later on.
    return energyFactor * lightScatter * viewScatter;
}
