#include <../include/common_constants_and_functions.glsl>

layout(location = 0) uniform sampler2D sampler_base_color; // texture "texture_base_color"

#include <../include/material.glsl>

in shared_data
{
    vec2 texcoord;
} fs_in;

out float depth;

void main()
{
    vec4 color = base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;
    if(alpha_mode == 2 && color.a < 1.0 - 1e-5)
        discard;
    if(alpha_mode == 3 && alpha_dither(gl_FragCoord.xy, sqrt(color.a)))
        discard;

    depth = gl_FragDepth;
}