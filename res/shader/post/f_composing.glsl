#include <../include/scene_post.glsl>

vec4 tonemap_with_gamma_correction(in vec4 color);
vec3 sharpen(in vec3 color, in float strength);

void main()
{
    float depth  = texture(sampler_geometry_depth_input, texcoord).r;
    gl_FragDepth = depth; // pass through for debug drawer (atm).

    bool no_correction = debug_view_enabled; // TODO Paul: This is weird.

    vec4 color =  texture(sampler_hdr_input, texcoord);
    color.rgb = sharpen(color.rgb, 0.025);
    color = no_correction ? color : tonemap_with_gamma_correction(color);


    frag_color = color;
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

vec3 adjust_contrast(in vec3 color, in float value) {
  return 0.5 + (1.0 + value) * (color - 0.5);
}

vec3 sharpen(in vec3 color, in float strength)
{
    vec3 f = textureOffset(sampler_hdr_input, texcoord, ivec2(-1,-1)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2( 0,-1)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2( 1,-1)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2(-1, 0)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2( 0, 0)).rgb *   9. +
             textureOffset(sampler_hdr_input, texcoord, ivec2( 1, 0)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2(-1, 1)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2( 0, 1)).rgb *  -1. +
             textureOffset(sampler_hdr_input, texcoord, ivec2( 1, 1)).rgb *  -1. ;
    return mix(color, f ,strength);
}

vec4 tonemap_with_gamma_correction(in vec4 color)
{
    // tonemapping // TODO Paul: There is room for improvement. Gamma parameter?
    const float W = 11.2;
    vec3 outcol = uncharted2_tonemap(color.rgb * camera_exposure * 2.0);
    outcol /= uncharted2_tonemap(vec3(W));
    outcol = adjust_contrast(outcol, 0.025);
    return linear_to_srgb(vec4(outcol, color.a)); // gamma correction.
}
