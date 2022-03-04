#include <../include/common_constants_and_functions.glsl>

out vec4 frag_color;

in noperspective vec2 texcoord;

layout(binding = 0) uniform sampler2D sampler_input; // texture "texture_input"

layout(binding = 1, std140) uniform fxaa_data
{
    vec2 inverse_screen_size;
    float subpixel_filter;
};

#define FXAA_PC 1
#define FXAA_GLSL_130 1
#define FXAA_QUALITY__PRESET 39
#define FXAA_GREEN_AS_LUMA 1

#include <../include/fxaa.glsl>

const float edge_threshold = 0.166;
const float edge_threshold_min = 0.0833;

void main()
{
    frag_color = FxaaPixelShader(texcoord, vec4(0), sampler_input, sampler_input, sampler_input, inverse_screen_size, vec4(0), vec4(0), vec4(0), subpixel_filter, edge_threshold, edge_threshold_min, 0, 0, 0, vec4(0));
}
