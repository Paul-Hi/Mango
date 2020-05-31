#version 430 core

out vec4 frag_color;

in vec3 shared_texcoord;

layout (location = 2, binding = 0) uniform samplerCube skybox;
layout (location = 3) uniform float mip_level;

vec3 uncharted2_tonemap(vec3 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color * (A * color + C * B) + D * E)/(color * (A * color + B) + D * F)) - E / F;
}

vec4 linear_to_srgb(in vec4 linear)
{
    return vec4(pow(linear.rgb, vec3(1.0 / 2.2)), linear.a);
}

vec4 tonemap_with_gamma_correction(in vec4 color)
{
    // tonemapping // TODO Paul: There is room for improvement. Exposure and gamma parameters.
    const float W = 11.2;
    vec3 outcol = uncharted2_tonemap(color.rgb * 2.0);
    outcol /= uncharted2_tonemap(vec3(W));
    return linear_to_srgb(vec4(outcol, color.a)); // gamma correction.
}


void main()
{
    vec3 color = tonemap_with_gamma_correction(textureLod(skybox, shared_texcoord, mip_level)).rgb;

    frag_color = vec4(color, 1.0);
}
