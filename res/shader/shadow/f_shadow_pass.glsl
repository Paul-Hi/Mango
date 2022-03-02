#include <../include/common_constants_and_functions.glsl>

#define BIND_MATERIAL_DATA_BUFFER
#define BIND_INSTANCE_DRAW_DATA_BUFFER

#include <../include/binding_data.glsl>

layout(location = GEOMETRY_TEXTURE_SAMPLER_BASE_COLOR) uniform sampler2D sampler_base_color; // texture "texture_base_color"

in shared_data
{
    vec2 texcoord;
    flat uint draw_id;
} fs_in;

out float depth;

void main()
{
    int material_id = instance_data_array[fs_in.draw_id].model_index;
    per_material_data data = material_data_array[material_id];

    vec4 color = data.base_color_texture ? texture(sampler_base_color, fs_in.texcoord) : data.base_color;
    if(data.alpha_mode == 1 && alpha_dither(gl_FragCoord.xy, sqrt(color.a)))
        discard;
    if(data.alpha_mode == 2 && color.a <= data.alpha_cutoff)
        discard;
    if(data.alpha_mode == 3 && color.a < 1.0 - 1e-5)
        discard;

    depth = gl_FragDepth;
}