#include <include/common_constants_and_functions.glsl>
#include <include/bindings.glsl>

#define BIND_HIERARCHICAL_Z_DATA_BUFFER

#include <include/binding_data.glsl>

void main()
{
    ivec2 texelcoord = ivec2(gl_FragCoord);
    ivec2 last_mip_texelcoord = 2 * texelcoord;
    ivec2 last_size = ivec2(sizes.zw);

    vec4 depth_values;
    depth_values.r = texelFetch(depth_buffer_sampler_in, last_mip_texelcoord, last_mip).r;
    depth_values.y = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(0, 1), ivec2(0), last_size), last_mip).r;
    depth_values.z = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(1, 0), ivec2(0), last_size), last_mip).r;
    depth_values.w = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(1, 1), ivec2(0), last_size), last_mip).r;

    float max_depth = max(depth_values.x, max(depth_values.y, max(depth_values.z, depth_values.w)));

    bool odd_x = ((last_size.x & 1) != 0);
    bool odd_y = ((last_size.y & 1) != 0);
    if(odd_x)
    {
        vec2 extra_col;
        extra_col.x = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(2, 0), ivec2(0), last_size), last_mip).r;
        extra_col.y = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(2, 1), ivec2(0), last_size), last_mip).r;
        max_depth = max(max_depth, max(extra_col.x, extra_col.y));
        if(odd_y)
        {
            float corner = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(2, 2), ivec2(0), last_size), last_mip).r;
            max_depth = max(max_depth, corner);
        }
    }
    if(odd_y)
    {
        vec2 extra_row;
        extra_row.x = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(0, 2), ivec2(0), last_size), last_mip).r;
        extra_row.y = texelFetch(depth_buffer_sampler_in, clamp(last_mip_texelcoord + ivec2(1, 2), ivec2(0), last_size), last_mip).r;
        max_depth = max(max_depth, max(extra_row.x, extra_row.y));
    }

    gl_FragDepth = max_depth;
}
