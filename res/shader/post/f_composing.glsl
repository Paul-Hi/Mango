#include <../include/composing.glsl>

vec4 tonemap_with_gamma_correction(in vec4 color);
vec3 mult_color(in vec3 color, in vec3 color_factor);
vec3 adjust_contrast(in vec3 color, in vec3 contrast);
vec3 saturate_color(in vec3 color, in vec3 saturation);
vec3 lift_gamma_gain(in vec3 color, in vec3 lift, in vec3 gamma, in vec3 gain);

const mat3 srgb_to_acescg = mat3(
        0.61319,  0.07021,  0.02062,
        0.33951,  0.91634,  0.10957,
        0.04737,  0.01345,  0.86961
     );

const float middle_gray_acescg = 0.18;

float luma_acescg(in vec3 color)
{
    return dot(color, vec3(0.272229, 0.674082, 0.0536895));
}

void main()
{
    float depth  = texture(sampler_geometry_depth_input, texcoord).r;
    gl_FragDepth = depth; // pass through for debug drawer (atm).

    bool no_correction = debug_view_enabled; // TODO Paul: This is weird.

    vec4 color =  texture(sampler_hdr_input, texcoord);
    if (no_correction)
    {
        frag_color = color;
        return;
    }

    vec4 hdr_color = vec4(srgb_to_acescg * color.rgb, color.a);

    hdr_color.rgb *= (camera_exposure + exposure_bias * 0.001);

    hdr_color.rgb = mult_color(hdr_color.rgb, tint);

    hdr_color.rgb = adjust_contrast(hdr_color.rgb, contrast);

    hdr_color.rgb = saturate_color(hdr_color.rgb, saturation);

    hdr_color.rgb = lift_gamma_gain(hdr_color.rgb, lift, gamma, gain);

    frag_color = tonemap_with_gamma_correction(hdr_color); // we could ditch this for hdr displays
}

// Narkowicz Approximation
vec3 ACES(in vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

vec3 mult_color(in vec3 color, in vec3 color_factor)
{
    return color * color_factor;
}

vec3 adjust_contrast(in vec3 color, in vec3 contrast)
{
    return middle_gray_acescg + contrast * (color - middle_gray_acescg);
}

vec3 saturate_color(in vec3 color, in vec3 saturation)
{
    float luma = luma_acescg(color);
    return luma + saturation * (color - luma);
}

vec3 lift_gamma_gain(in vec3 color, in vec3 lift, in vec3 gamma, in vec3 gain)
{
    return lift + gain * pow(color, gamma);
}

vec4 tonemap_with_gamma_correction(in vec4 color)
{
    // tonemapping
    vec4 ldr = vec4(ACES(color.rgb), color.a);
    return linear_to_srgb(ldr); // gamma correction.
}
