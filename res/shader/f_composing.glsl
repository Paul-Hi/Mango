#version 430 core

const float PI = 3.1415926535897932384626433832795;
const float INV_PI = 1.0 / PI;

#define saturate(x) clamp(x, 0.0, 1.0)

out vec4 frag_color;

in vec2 texcoord;

layout(location = 0, binding = 0) uniform sampler2D hdr_input;
layout (location = 1) uniform float camera_exposure;

vec4 tonemap_with_gamma_correction(in vec4 color);
vec4 srgb_to_linear(in vec4 srgb);
vec4 linear_to_srgb(in vec4 linear);

void main()
{
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
    vec3 outcol = uncharted2_tonemap(color.rgb * camera_exposure * 2.0);
    outcol /= uncharted2_tonemap(vec3(W));
    return linear_to_srgb(vec4(outcol, color.a)); // gamma correction.
}

vec4 srgb_to_linear(in vec4 srgb)
{
    return vec4(pow(srgb.rgb, vec3(2.2)), srgb.a);
}

vec4 linear_to_srgb(in vec4 linear)
{
    return vec4(pow(linear.rgb, vec3(1.0 / 2.2)), linear.a);
}
